#include "sd_image.h"
#include "xil_io.h"
#include <string.h>

#define SPI_BASE        0x44A00000
#define SPI_SRR         (SPI_BASE + 0x40)
#define SPI_SPICR       (SPI_BASE + 0x60)
#define SPI_SPISR       (SPI_BASE + 0x64)
#define SPI_DTR         (SPI_BASE + 0x68)
#define SPI_DRR         (SPI_BASE + 0x6C)
#define SPI_SPISSR      (SPI_BASE + 0x70)

#define SPICR_SPE       (1 << 1)
#define SPICR_MASTER    (1 << 2)
#define SPICR_MANUAL    (1 << 7)
#define SPICR_MTI       (1 << 8)

#define SPISR_RX_EMPTY  (1 << 0)
#define SPISR_TX_EMPTY  (1 << 2)

static unsigned char *image_buffer = (unsigned char *)DDR2_IMAGE_BASE;

/* ===================== Capa SPI / SD (bajo nivel, sin cambios) ===================== */

static unsigned char spi_byte(unsigned char tx) {
    int t;

    while (!(Xil_In32(SPI_SPISR) & SPISR_RX_EMPTY))
        (void)Xil_In32(SPI_DRR);

    Xil_Out32(SPI_DTR, (unsigned int)tx);
    Xil_Out32(SPI_SPICR, SPICR_MASTER | SPICR_SPE | SPICR_MANUAL);

    t = 2000000;
    while ((Xil_In32(SPI_SPISR) & SPISR_RX_EMPTY) && --t);

    Xil_Out32(SPI_SPICR, SPICR_MASTER | SPICR_SPE | SPICR_MANUAL | SPICR_MTI);

    if (!t) return 0xFF;
    return (unsigned char)(Xil_In32(SPI_DRR) & 0xFF);
}

static unsigned char sd_cmd(unsigned char cmd, unsigned int arg, unsigned char crc) {
    unsigned char r;
    int i;

    spi_byte(0x40 | cmd);
    spi_byte((unsigned char)((arg >> 24) & 0xFF));
    spi_byte((unsigned char)((arg >> 16) & 0xFF));
    spi_byte((unsigned char)((arg >> 8) & 0xFF));
    spi_byte((unsigned char)(arg & 0xFF));
    spi_byte(crc | 0x01);

    for (i = 0; i < 10; i++) {
        r = spi_byte(0xFF);
        if (!(r & 0x80)) return r;
    }
    return 0xFF;
}

static int sd_read_block(unsigned int block_addr, unsigned char *buffer) {
    unsigned char r;
    int i;

    Xil_Out32(SPI_SPISSR, 0x00000000);
    spi_byte(0xFF);
    r = sd_cmd(17, block_addr, 0x00);
    Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);
    spi_byte(0xFF);

    if (r != 0x00) {
        return -1;
    }

    Xil_Out32(SPI_SPISSR, 0x00000000);
    for (i = 0; i < 100000; i++) {
        r = spi_byte(0xFF);
        if (r == 0xFE) break;
    }

    if (r != 0xFE) {
        Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);
        return -1;
    }

    for (i = 0; i < 512; i++) {
        buffer[i] = spi_byte(0xFF);
    }
    spi_byte(0xFF);
    spi_byte(0xFF);

    Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);
    spi_byte(0xFF);

    return 0;
}

int sd_image_init(void) {
    Xil_Out32(SPI_SRR, 0x0000000A);
    for (volatile int i = 0; i < 10000000; i++);

    Xil_Out32(SPI_SPICR, SPICR_MASTER | SPICR_SPE | SPICR_MANUAL | SPICR_MTI);
    Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);

    for (int i = 0; i < 10; i++)
        spi_byte(0xFF);

    Xil_Out32(SPI_SPISSR, 0x00000000);
    spi_byte(0xFF);
    unsigned char r = sd_cmd(0, 0x00000000, 0x95);
    Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);
    spi_byte(0xFF);

    if (r != 0x01) return -1;

    Xil_Out32(SPI_SPISSR, 0x00000000);
    spi_byte(0xFF);
    r = sd_cmd(8, 0x000001AA, 0x87);
    if (r == 0x01) {
        for (int i = 0; i < 4; i++)
            spi_byte(0xFF);
    }
    Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);
    spi_byte(0xFF);

    if (r != 0x01 && r != 0x05) return -1;

    int is_hcs = (r == 0x01);

    unsigned int acmd41_arg = is_hcs ? 0x40000000 : 0x00000000;
    for (int i = 0; i < 1000; i++) {
        Xil_Out32(SPI_SPISSR, 0x00000000);
        spi_byte(0xFF);
        r = sd_cmd(55, 0x00000000, 0x00);
        Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);
        spi_byte(0xFF);

        if (r > 0x01) return -1;

        Xil_Out32(SPI_SPISSR, 0x00000000);
        spi_byte(0xFF);
        r = sd_cmd(41, acmd41_arg, 0x00);
        Xil_Out32(SPI_SPISSR, 0xFFFFFFFF);
        spi_byte(0xFF);

        if (r == 0x00) break;
        if (r != 0x01) return -1;

        for (volatile int i = 0; i < 100000; i++);
    }

    if (r != 0x00) return -1;

    return 0;
}

/* ===================== Capa FAT16 / FAT32 (con soporte de MBR) ===================== */

typedef struct {
    unsigned int bytes_per_sector;
    unsigned int sectors_per_cluster;
    unsigned int reserved_sectors;
    unsigned int num_fats;
    unsigned int root_entries;
    unsigned int fat_size;
    unsigned int root_cluster;
    unsigned int first_fat_sector;
    unsigned int root_dir_sector;
    unsigned int root_dir_sectors;
    unsigned int first_data_sector;
    unsigned int partition_base;  /* LBA absoluto donde arranca el volumen FAT (0 si no hay MBR) */
    int is_fat32;
} fat_info_t;

static fat_info_t fs;

static unsigned short rd16(const unsigned char *p) {
    return (unsigned short)(p[0] | (p[1] << 8));
}

static unsigned int rd32(const unsigned char *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

/* Lee un sector relativo al inicio del VOLUMEN (suma partition_base automaticamente) */
static int read_vol_sector(unsigned int rel_sector, unsigned char *buf) {
    return sd_read_block(fs.partition_base + rel_sector, buf);
}

/* Interpreta un buffer de 512 bytes como BPB. Devuelve 0 si los campos tienen
 * pinta de ser un BPB real (y no, por ejemplo, un MBR malinterpretado). */
static int parse_bpb(const unsigned char *boot_sector) {
    fs.bytes_per_sector    = rd16(&boot_sector[11]);
    fs.sectors_per_cluster = boot_sector[13];
    fs.reserved_sectors    = rd16(&boot_sector[14]);
    fs.num_fats            = boot_sector[16];
    fs.root_entries        = rd16(&boot_sector[17]);

    unsigned int fat_size_16 = rd16(&boot_sector[22]);
    unsigned int fat_size_32 = rd32(&boot_sector[36]);

    if (fat_size_16 != 0) {
        fs.is_fat32     = 0;
        fs.fat_size     = fat_size_16;
        fs.root_cluster = 0;
    } else {
        fs.is_fat32     = 1;
        fs.fat_size     = fat_size_32;
        fs.root_cluster = rd32(&boot_sector[44]);
    }

    int bps_ok = (fs.bytes_per_sector == 512 || fs.bytes_per_sector == 1024 ||
                  fs.bytes_per_sector == 2048 || fs.bytes_per_sector == 4096);
    int nf_ok  = (fs.num_fats == 1 || fs.num_fats == 2);
    int spc_ok = (fs.sectors_per_cluster != 0);

    return (bps_ok && nf_ok && spc_ok) ? 0 : -1;
}

/* Monta el sistema de archivos. Detecta automaticamente si el sector 0 es
 * un BPB directo (tarjeta "superfloppy", sin tabla de particiones) o un MBR
 * (tabla de particiones), en cuyo caso busca la primera particion y lee el
 * BPB real desde ahi. Silencioso: no escribe nada en pantalla. */
static int fat_mount(void) {
    unsigned char sector0[512];

    fs.partition_base = 0;

    if (sd_read_block(0, sector0) != 0) return -1;
    if (sector0[510] != 0x55 || sector0[511] != 0xAA) return -1;

    if (parse_bpb(sector0) != 0) {
        unsigned char ptype = sector0[446 + 4];
        unsigned int  plba  = rd32(&sector0[446 + 8]);

        if (ptype == 0x00 || plba == 0) return -1;

        unsigned char sector_p[512];
        if (sd_read_block(plba, sector_p) != 0) return -1;
        if (sector_p[510] != 0x55 || sector_p[511] != 0xAA || parse_bpb(sector_p) != 0) return -1;

        fs.partition_base = plba;
    }

    fs.first_fat_sector  = fs.reserved_sectors;
    fs.root_dir_sectors  = ((fs.root_entries * 32) + (fs.bytes_per_sector - 1)) / fs.bytes_per_sector;
    fs.root_dir_sector   = fs.reserved_sectors + (fs.num_fats * fs.fat_size);
    fs.first_data_sector = fs.root_dir_sector + fs.root_dir_sectors;

    return 0;
}

static unsigned int cluster_to_sector(unsigned int cluster) {
    return fs.first_data_sector + (cluster - 2) * fs.sectors_per_cluster;
}

static int is_eoc(unsigned int val) {
    if (fs.is_fat32) return val >= 0x0FFFFFF8;
    return val >= 0xFFF8;
}

static unsigned int get_next_cluster(unsigned int cluster) {
    unsigned char sector_buf[512];
    unsigned int fat_offset = fs.is_fat32 ? (cluster * 4) : (cluster * 2);
    unsigned int fat_sector = fs.first_fat_sector + (fat_offset / fs.bytes_per_sector);
    unsigned int ent_offset = fat_offset % fs.bytes_per_sector;

    if (read_vol_sector(fat_sector, sector_buf) != 0) {
        return 0x0FFFFFFF;
    }

    if (fs.is_fat32) {
        return rd32(&sector_buf[ent_offset]) & 0x0FFFFFFF;
    }
    return rd16(&sector_buf[ent_offset]);
}

static int scan_dir_sector(const unsigned char *target_name, const unsigned char *dir_sector,
                            unsigned int *start_cluster, unsigned int *file_size, int *end_of_dir) {
    unsigned int entries_per_sector = fs.bytes_per_sector / 32;

    for (unsigned int j = 0; j < entries_per_sector; j++) {
        const unsigned char *entry = dir_sector + (j * 32);

        if (entry[0] == 0x00) { *end_of_dir = 1; return 0; }
        if (entry[0] == 0xE5) continue;
        if (entry[11] == 0x0F) continue;
        if (entry[11] & 0x08) continue;
        if (entry[11] & 0x10) continue;

        int match = 1;
        for (int k = 0; k < 11; k++) {
            if (entry[k] != target_name[k]) { match = 0; break; }
        }

        if (match) {
            unsigned int cl_hi = rd16(&entry[20]);
            unsigned int cl_lo = rd16(&entry[26]);
            *start_cluster = (cl_hi << 16) | cl_lo;
            *file_size = rd32(&entry[28]);
            return 1;
        }
    }
    return 0;
}

static int fat_find_file(const char *filename, unsigned int *start_cluster, unsigned int *file_size) {
    unsigned char target_name[11];
    unsigned char sector_buf[512];
    int dot_pos = -1;
    int name_len = 0;

    for (int i = 0; i < 11; i++) target_name[i] = ' ';

    for (int i = 0; filename[i]; i++) {
        if (filename[i] == '.') { dot_pos = i; break; }
        name_len++;
    }

    for (int i = 0; i < name_len && i < 8; i++) {
        char c = filename[i];
        target_name[i] = (c >= 'a' && c <= 'z') ? (unsigned char)(c - 'a' + 'A') : (unsigned char)c;
    }

    if (dot_pos >= 0) {
        int ext_idx = dot_pos + 1;
        for (int i = 0; i < 3 && filename[ext_idx]; i++, ext_idx++) {
            char c = filename[ext_idx];
            target_name[8 + i] = (c >= 'a' && c <= 'z') ? (unsigned char)(c - 'a' + 'A') : (unsigned char)c;
        }
    }

    if (!fs.is_fat32) {
        for (unsigned int i = 0; i < fs.root_dir_sectors; i++) {
            if (read_vol_sector(fs.root_dir_sector + i, sector_buf) != 0) return -1;
            int end_of_dir = 0;
            if (scan_dir_sector(target_name, sector_buf, start_cluster, file_size, &end_of_dir)) return 0;
            if (end_of_dir) return -1;
        }
        return -1;
    } else {
        unsigned int cluster = fs.root_cluster;
        while (cluster >= 2 && !is_eoc(cluster)) {
            unsigned int first_sector = cluster_to_sector(cluster);
            for (unsigned int s = 0; s < fs.sectors_per_cluster; s++) {
                if (read_vol_sector(first_sector + s, sector_buf) != 0) return -1;
                int end_of_dir = 0;
                if (scan_dir_sector(target_name, sector_buf, start_cluster, file_size, &end_of_dir)) return 0;
                if (end_of_dir) return -1;
            }
            cluster = get_next_cluster(cluster);
        }
        return -1;
    }
}

int sd_image_load(const char *filename) {
    unsigned char *ddr2 = (unsigned char *)DDR2_IMAGE_BASE;
    unsigned char sector_buf[512];
    unsigned int start_cluster, file_size;

    if (fat_mount() != 0) return -1;
    if (fat_find_file(filename, &start_cluster, &file_size) != 0) return -1;
    if (start_cluster < 2) return -1;

    unsigned int to_copy = (file_size != 0 && file_size < IMAGE_SIZE) ? file_size : IMAGE_SIZE;
    unsigned int bytes_copied = 0;
    unsigned int cluster = start_cluster;

    while (cluster >= 2 && !is_eoc(cluster) && bytes_copied < to_copy) {
        unsigned int first_sector = cluster_to_sector(cluster);

        for (unsigned int s = 0; s < fs.sectors_per_cluster && bytes_copied < to_copy; s++) {
            if (read_vol_sector(first_sector + s, sector_buf) != 0) return -1;
            unsigned int chunk = (to_copy - bytes_copied > 512) ? 512 : (to_copy - bytes_copied);
            memcpy(ddr2 + bytes_copied, sector_buf, chunk);
            bytes_copied += chunk;
        }

        cluster = get_next_cluster(cluster);
    }

    return (bytes_copied >= to_copy) ? 0 : -1;
}

unsigned char* sd_image_get_buffer(void) {
    return image_buffer;
}