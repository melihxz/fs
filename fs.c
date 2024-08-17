#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SECTOR_SIZE 512
#define FAT12_SECTORS 9
#define FAT16_SECTORS 18
#define FAT32_SECTORS 36
#define ROOT_DIR_ENTRIES 512
#define NTFS_MFT_SIZE 4096
#define NTFS_CLUSTER_SIZE 4096
#define NTFS_MFT_RECORD_SIZE 1024
#define MAX_FILENAME_LEN 256

// FAT Definitions
typedef struct
{
    uint8_t jump[3];
    uint8_t oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;  // FAT12/FAT16
    uint16_t total_sectors; // FAT16
    uint8_t media_descriptor;
    uint16_t fat_size; // FAT16
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_large; // FAT32
    uint32_t fat_size_large;      // FAT32
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster; // FAT32
    uint16_t fsinfo_sectors;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t file_system_type[8];
} __attribute__((packed)) FAT_BootSector;

typedef struct
{
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t high_cluster;
    uint16_t last_modification_time;
    uint16_t last_modification_date;
    uint16_t low_cluster;
    uint32_t file_size;
} __attribute__((packed)) FAT_DirEntry;

typedef struct
{
    uint32_t start_cluster;
    uint32_t size;
    uint8_t *data;
} FAT_File;

typedef struct
{
    FAT_BootSector boot_sector;
    FAT_DirEntry *root_dir;
    uint8_t *fat;
    uint8_t *data_area;
    uint32_t fat_size;
    uint32_t root_dir_start;
    uint32_t data_area_start;
} FAT_FS;

// NTFS Definitions
typedef struct
{
    uint8_t jump[3];
    uint8_t oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t zero1;
    uint8_t zero2;
    uint16_t zero3;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors;
    uint32_t sectors_per_fat_large;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t file_system_type[8];
} __attribute__((packed)) NTFS_BootSector;

typedef struct
{
    uint32_t magic;
    uint16_t update_sequence_offset;
    uint16_t update_sequence_size;
    uint64_t log_file_start;
    uint64_t sequence_number;
    uint64_t log_file_seq_number;
    uint32_t volume_flags;
    uint32_t max_component_name_length;
    uint32_t file_system_attributes;
    uint64_t root_directory_start;
    uint64_t data_file_start;
} __attribute__((packed)) NTFS_MFT_Entry;

typedef struct
{
    NTFS_BootSector boot_sector;
    NTFS_MFT_Entry *mft;
    uint8_t *data_area;
    uint64_t mft_size;
    uint64_t data_area_size;
} NTFS_FS;

// Utility Functions
void handle_error(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void print_fat_boot_info(FAT_BootSector *boot_sector)
{
    printf("FAT Boot Sector:\n");
    printf("OEM ID: %.8s\n", boot_sector->oem_id);
    printf("Bytes per Sector: %u\n", boot_sector->bytes_per_sector);
    printf("Sectors per Cluster: %u\n", boot_sector->sectors_per_cluster);
    printf("Reserved Sectors: %u\n", boot_sector->reserved_sectors);
    printf("FAT Count: %u\n", boot_sector->fat_count);
    printf("Root Entries: %u\n", boot_sector->root_entries);
    printf("Total Sectors: %u\n", boot_sector->total_sectors_large);
    printf("Media Descriptor: %02X\n", boot_sector->media_descriptor);
    printf("FAT Size: %u\n", boot_sector->fat_size_large);
    printf("Volume ID: %08X\n", boot_sector->volume_id);
}

void print_fat_root_directory(FAT_DirEntry *root_dir)
{
    for (int i = 0; i < ROOT_DIR_ENTRIES; i++)
    {
        FAT_DirEntry *entry = &root_dir[i];
        if (entry->filename[0] == 0x00)
            break;
        printf("Filename: %.8s.%.3s\n", entry->filename, entry->extension);
        printf("File Size: %u\n", entry->file_size);
        printf("Attributes: %02X\n", entry->attributes);
        printf("Creation Date: %04X\n", entry->creation_date);
        printf("Last Modification Date: %04X\n", entry->last_modification_date);
    }
}

void print_ntfs_boot_info(NTFS_BootSector *boot_sector)
{
    printf("NTFS Boot Sector:\n");
    printf("OEM ID: %.8s\n", boot_sector->oem_id);
    printf("Bytes per Sector: %u\n", boot_sector->bytes_per_sector);
    printf("Sectors per Cluster: %u\n", boot_sector->sectors_per_cluster);
    printf("Reserved Sectors: %u\n", boot_sector->reserved_sectors);
    printf("Volume ID: %08X\n", boot_sector->volume_id);
}

void print_ntfs_data_area(uint8_t *data_area, uint64_t size)
{
    for (uint64_t i = 0; i < size; i++)
    {
        printf("%02X ", data_area[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
}

// FAT Functions
void read_fat_boot_sector(FILE *disk, FAT_FS *fs)
{
    if (fseek(disk, 0, SEEK_SET) != 0)
        handle_error("Error seeking to boot sector");
    if (fread(&fs->boot_sector, sizeof(FAT_BootSector), 1, disk) != 1)
        handle_error("Error reading boot sector");
}

void read_fat_fat(FILE *disk, FAT_FS *fs)
{
    fs->fat_size = fs->boot_sector.fat_size_large * SECTOR_SIZE;
    fs->fat = malloc(fs->fat_size);
    if (fs->fat == NULL)
        handle_error("Memory allocation failed for FAT");
    if (fseek(disk, fs->boot_sector.reserved_sectors * SECTOR_SIZE, SEEK_SET) != 0)
        handle_error("Error seeking to FAT");
    if (fread(fs->fat, fs->fat_size, 1, disk) != 1)
        handle_error("Error reading FAT");
}

void read_fat_root_directory(FILE *disk, FAT_FS *fs)
{
    fs->root_dir_start = (fs->boot_sector.reserved_sectors + fs->boot_sector.fat_size_large * fs->boot_sector.fat_count) * SECTOR_SIZE;
    fs->root_dir = malloc(ROOT_DIR_ENTRIES * sizeof(FAT_DirEntry));
    if (fs->root_dir == NULL)
        handle_error("Memory allocation failed for root directory");
    if (fseek(disk, fs->root_dir_start, SEEK_SET) != 0)
        handle_error("Error seeking to root directory");
    if (fread(fs->root_dir, ROOT_DIR_ENTRIES * sizeof(FAT_DirEntry), 1, disk) != 1)
        handle_error("Error reading root directory");
}

void read_fat_data_area(FILE *disk, FAT_FS *fs)
{
    fs->data_area_start = fs->root_dir_start + ROOT_DIR_ENTRIES * sizeof(FAT_DirEntry);
    fs->data_area = malloc(SECTOR_SIZE * 1024); // Example: 1 MB data area
    if (fs->data_area == NULL)
        handle_error("Memory allocation failed for data area");
    if (fseek(disk, fs->data_area_start, SEEK_SET) != 0)
        handle_error("Error seeking to data area");
    if (fread(fs->data_area, SECTOR_SIZE * 1024, 1, disk) != 1)
        handle_error("Error reading data area");
}

void read_fat_file(FILE *disk, FAT_FS *fs, FAT_DirEntry *entry)
{
    uint32_t start_cluster = ((uint32_t)entry->high_cluster << 16) | entry->low_cluster;
    uint32_t size = entry->file_size;
    uint8_t *file_data = malloc(size);
    if (file_data == NULL)
        handle_error("Memory allocation failed for file data");
    if (fseek(disk, fs->data_area_start + (start_cluster - 2) * fs->boot_sector.sectors_per_cluster * SECTOR_SIZE, SEEK_SET) != 0)
        handle_error("Error seeking to file data");
    if (fread(file_data, size, 1, disk) != 1)
        handle_error("Error reading file data");
    // Process file_data as needed
    free(file_data);
}

// NTFS Functions
void read_ntfs_boot_sector(FILE *disk, NTFS_FS *fs)
{
    if (fseek(disk, 0, SEEK_SET) != 0)
        handle_error("Error seeking to boot sector");
    if (fread(&fs->boot_sector, sizeof(NTFS_BootSector), 1, disk) != 1)
        handle_error("Error reading boot sector");
}

void read_ntfs_mft(FILE *disk, NTFS_FS *fs)
{
    fs->mft_size = NTFS_MFT_SIZE;
    fs->mft = malloc(NTFS_MFT_SIZE);
    if (fs->mft == NULL)
        handle_error("Memory allocation failed for MFT");
    if (fseek(disk, fs->boot_sector.reserved_sectors * SECTOR_SIZE, SEEK_SET) != 0)
        handle_error("Error seeking to MFT");
    if (fread(fs->mft, NTFS_MFT_SIZE, 1, disk) != 1)
        handle_error("Error reading MFT");
}

void read_ntfs_data_area(FILE *disk, NTFS_FS *fs)
{
    fs->data_area_size = NTFS_CLUSTER_SIZE;
    fs->data_area = malloc(NTFS_CLUSTER_SIZE);
    if (fs->data_area == NULL)
        handle_error("Memory allocation failed for data area");
    if (fseek(disk, fs->boot_sector.reserved_sectors * SECTOR_SIZE + NTFS_MFT_SIZE, SEEK_SET) != 0)
        handle_error("Error seeking to data area");
    if (fread(fs->data_area, NTFS_CLUSTER_SIZE, 1, disk) != 1)
        handle_error("Error reading data area");
}

void read_ntfs_file(FILE *disk, NTFS_FS *fs, uint64_t mft_index)
{
    uint64_t mft_offset = fs->boot_sector.reserved_sectors * SECTOR_SIZE + mft_index * NTFS_MFT_RECORD_SIZE;
    if (fseek(disk, mft_offset, SEEK_SET) != 0)
        handle_error("Error seeking to MFT record");
    NTFS_MFT_Entry mft_entry;
    if (fread(&mft_entry, sizeof(NTFS_MFT_Entry), 1, disk) != 1)
        handle_error("Error reading MFT record");
    // Process mft_entry to read file information
}

void write_ntfs_file(FILE *disk, uint64_t start_cluster, uint8_t *data, uint32_t size)
{
    if (fseek(disk, start_cluster * NTFS_CLUSTER_SIZE, SEEK_SET) != 0)
        handle_error("Error seeking to file data");
    if (fwrite(data, size, 1, disk) != 1)
        handle_error("Error writing file data");
}

// Main Function
int main()
{
    FAT_FS fat_fs = {0};
    NTFS_FS ntfs_fs = {0};
    const char *disk_image = "combined.img";

    FILE *disk = fopen(disk_image, "rb+");
    if (disk == NULL)
        handle_error("Failed to open disk image");

    // Read FAT and NTFS parts
    read_fat_boot_sector(disk, &fat_fs);
    read_fat_fat(disk, &fat_fs);
    read_fat_root_directory(disk, &fat_fs);
    read_fat_data_area(disk, &fat_fs);

    read_ntfs_boot_sector(disk, &ntfs_fs);
    read_ntfs_mft(disk, &ntfs_fs);
    read_ntfs_data_area(disk, &ntfs_fs);

    // Print information
    print_fat_boot_info(&fat_fs.boot_sector);
    print_fat_root_directory(fat_fs.root_dir);
    print_ntfs_boot_info(&ntfs_fs.boot_sector);
    print_ntfs_data_area(ntfs_fs.data_area, ntfs_fs.data_area_size);

    // Example File Read
    FAT_DirEntry example_file = {0};
    read_fat_file(disk, &fat_fs, &example_file);
    read_ntfs_file(disk, &ntfs_fs, 0);

    // Free resources
    free(fat_fs.fat);
    free(fat_fs.root_dir);
    free(fat_fs.data_area);
    free(ntfs_fs.mft);
    free(ntfs_fs.data_area);

    fclose(disk);

    return 0;
}
