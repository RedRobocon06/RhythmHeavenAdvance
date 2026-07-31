#include "memory.h"
#include "src/lib_sram.h"

static u16 sEWRAMSaveBufferStart;
static u16 sEWRAMSaveBufferEnd;
static u16 sEWRAMMemoryHeapStart;
static u16 sEWRAMMemoryHeapLength;
static s32 unused_03000094; // unknown

extern u8 *save_memory_base; // CartRAMBase (0x0E000000)
extern u8 *backup_save_memory_base; // CartRAMBase + 0x4000 (0x0E004000)
extern char D_08935fbc[]; // "RIQ"
extern char D_08935fc4[]; // "CAL"


/* SAVE/MEMORY */

// Initiate the size and positions of the save buffer and memory heap.
void init_ewram(void) {
    u16 ewramStart = 4;
    u16 ewramEnd = 0x10000 - 4;
    u16 saveBufferLength = SAVE_BUFFER_SIZE / 4;
	u16 saveEnd, heapLength;

    sEWRAMSaveBufferStart = ewramStart;
    saveEnd = ewramStart + saveBufferLength;
	heapLength = ewramEnd - saveBufferLength;
    sEWRAMSaveBufferEnd = saveEnd;

    sEWRAMMemoryHeapStart = saveEnd;
    sEWRAMMemoryHeapLength = heapLength;
}


void *get_save_buffer_start(void) {
	return (void *)(ExternWorkRAMBase + (sEWRAMSaveBufferStart * 4));
}


void *get_save_buffer_end(void) {
	return (void *)(ExternWorkRAMBase + (sEWRAMSaveBufferEnd * 4));
}


void *get_memory_heap_start(void) {
	return (void *)(ExternWorkRAMBase + (sEWRAMMemoryHeapStart * 4));
}


u32 get_memory_heap_length(void) {
	return sEWRAMMemoryHeapLength * 4;
}


s32 generate_save_buffer_checksum(s32 *buffer, u32 bufferSize) {
	u32 length, remainder;
	u32 i;
	s32 total = 0;

    bufferSize /= 4;
    length = bufferSize / 16;
    remainder = bufferSize - (length * 16);

	for (i = 0; i < length; i++) {
		total += buffer[0];
		total += buffer[1];
		total += buffer[2];
		// total += buffer[3]; // lmao why is this missing
		total += buffer[4];
		total += buffer[5];
		total += buffer[6];
		total += buffer[7];
		total += buffer[8];
		total += buffer[9];
		total += buffer[10];
		total += buffer[11];
		total += buffer[12];
		total += buffer[13];
		total += buffer[14];
		total += buffer[15];
        buffer += 16;
	}

	for (i = 0; i < remainder; i++) {
		total += buffer[i];
	}

	return total;
}


// Init.
void init_save_buffer(void) {
    set_sram_fast_func();
    D_030046a8 = get_save_buffer_start();
}


void clear_save_data(void) {
    struct SaveBuffer *buffer = D_030046a8;

    dma3_fill(0, buffer, SAVE_BUFFER_SIZE, 0x20, 0x100);
    strcpy(buffer->header.RIQ, D_08935fbc);
    buffer->header.bufferSize = SAVE_BUFFER_SIZE;
    buffer->header.checksum = 0;
    buffer->header.unkC = 0x26040000;
    reset_game_save_data();
}


s32 copy_to_save_buffer(u8 *cartRAM) {
    struct SaveBuffer *buffer = D_030046a8;

    read_sram_fast(cartRAM, (u8 *)buffer, SAVE_BUFFER_SIZE);

    if (strncmp(buffer->header.RIQ, D_08935fbc, 4) != 0) {
        return 1;
    }

    if (calculate_save_buffer_checksum(buffer) != buffer->header.checksum) {
        return 2;
    }

    if (!extra_save_data_is_valid(&buffer->data.extraData)) {
        reset_extra_save_data_defaults(&buffer->data);
    }

    if(buffer->data.totalMedals > MAX_MEDALS) {
        buffer->data.totalMedals = MAX_MEDALS;
    }
    
    SET_ADVANCE_FLAG(buffer->data.advanceFlags, ADVANCE_FLAG_SAVE_CONVERTED);

    return 0;
}


s32 copy_sram_to_save_buffer(void) {
	return copy_to_save_buffer(save_memory_base);
}


s32 copy_sram_backup_to_save_buffer(void) {
	return copy_to_save_buffer(backup_save_memory_base);
}


void flush_save_buffer(u8 *cartRAM) {
}


s32 get_offset_from_save_buffer(void *buffer) {
	return (u32)buffer - (u32)D_030046a8;
}


void write_save_buffer_header_to_sram(u8 *cartRAM) {
    struct SaveBuffer *buffer = D_030046a8;

    update_save_buffer_header(buffer);
    write_sram_fast((u8 *)buffer, cartRAM, sizeof(buffer->header));

    if (sSramSaveWriteState.active && (sSramSaveWriteState.cartRAM == cartRAM)) {
        sSramSaveWriteState.active = FALSE;
    }
}


void write_save_buffer_data_to_sram(u8 *buffer, u32 size) {
}


void flush_save_buffer_to_sram(void) {
}


void flush_save_buffer_to_sram_backup(void) {
	flush_save_buffer(backup_save_memory_base);
}


void update_save_buffer_sram_writes(void) {
}


void finish_save_buffer_sram_writes(void) {
}


void func_080009c8_stub(void) {
}


void func_080009cc_stub(void) {
}


s32 func_080009d0(s16 *arg1) {
    if ((strncmp(arg1, D_08935fc4, 4) == 0) && (arg1[2] == arg1[3])) {
        return arg1[2];
    }
    return 0;
}


s32 func_080009fc(void) {
	return 0;
}

u8 get_level_state(struct TengokuSaveData *data, u32 levelID) {
    if (levelID >= TOTAL_BASE_LEVELS){
        return data->extraData.extraLevelStates[levelID - TOTAL_BASE_LEVELS];
    }
    return data->levelStates[levelID];
}

u16 get_level_score(struct TengokuSaveData *data, u32 levelID) {
    if (levelID >= TOTAL_BASE_LEVELS){
        return data->extraData.extraLevelScores[levelID - TOTAL_BASE_LEVELS];
    }
    return data->levelScores[levelID];
}

u8 get_level_total_plays(struct TengokuSaveData *data, u32 levelID) {
    if (levelID >= TOTAL_BASE_LEVELS){
        return data->extraData.extraLevelTotalPlays[levelID - TOTAL_BASE_LEVELS];
    }
    return data->levelTotalPlays[levelID];
}

u8 get_level_first_ok(struct TengokuSaveData *data, u32 levelID) {
    if (levelID >= TOTAL_BASE_LEVELS){
        return data->extraData.extraLevelFirstOK[levelID - TOTAL_BASE_LEVELS];
    }
    return data->levelFirstOK[levelID];
}

u8 get_level_first_superb(struct TengokuSaveData *data, u32 levelID) {
    if (levelID >= TOTAL_BASE_LEVELS){
        return data->extraData.extraLevelFirstSuperb[levelID - TOTAL_BASE_LEVELS];
    }
    return data->levelFirstSuperb[levelID];
}

u8 get_campaign_cleared(struct TengokuSaveData *data, u32 campaignID) {
    if (campaignID >= TOTAL_BASE_PERFECT_CAMPAIGNS){
        return data->extraData.extraCampaignsCleared[campaignID - TOTAL_BASE_PERFECT_CAMPAIGNS];
    }
    return data->campaignsCleared[campaignID];
}

static u32 count_cleared_campaigns(struct TengokuSaveData *data, u32 totalCampaigns) {
    u32 i;
    u32 totalCleared = 0;

    for (i = 0; i < totalCampaigns; i++) {
        if (get_campaign_cleared(data, i)) {
            totalCleared++;
        }
    }

    return totalCleared;
}

u32 get_total_base_cleared_campaigns(struct TengokuSaveData *data) {
    return count_cleared_campaigns(data, BASE_CAMPAIGN_MILESTONE_TOTAL);
}

u32 get_total_active_cleared_campaigns(struct TengokuSaveData *data) {
    return count_cleared_campaigns(data, ACTIVE_AVAILABLE_CAMPAIGNS);
}

u8 get_reading_material_unlocked(struct TengokuSaveData *data, u32 materialID) {
    if (materialID >= TOTAL_BASE_READING_MATERIALS) {
        return data->extraData.extraReadingMaterialUnlocked[materialID - TOTAL_BASE_READING_MATERIALS];
    }
    return data->readingMaterialUnlocked[materialID];
}

void set_level_state(struct TengokuSaveData *data, u32 levelID, u8 state) {
    if (levelID >= TOTAL_BASE_LEVELS){
        data->extraData.extraLevelStates[levelID - TOTAL_BASE_LEVELS] = state;
    } else {
        data->levelStates[levelID] = state;
    }
}

void set_level_score(struct TengokuSaveData *data, u32 levelID, u16 score) {
    if (levelID >= TOTAL_BASE_LEVELS){
        data->extraData.extraLevelScores[levelID - TOTAL_BASE_LEVELS] = score;
    } else {
        data->levelScores[levelID] = score;
    }
}

void set_level_total_plays(struct TengokuSaveData *data, u32 levelID, u8 totalPlays) {
    if (levelID >= TOTAL_BASE_LEVELS){
        data->extraData.extraLevelTotalPlays[levelID - TOTAL_BASE_LEVELS] = totalPlays;
    } else {
        data->levelTotalPlays[levelID] = totalPlays;
    }
}

void set_level_first_ok(struct TengokuSaveData *data, u32 levelID, u8 firstOK) {
    if (levelID >= TOTAL_BASE_LEVELS){
        data->extraData.extraLevelFirstOK[levelID - TOTAL_BASE_LEVELS] = firstOK;
    } else {
        data->levelFirstOK[levelID] = firstOK;
    }
}

void set_level_first_superb(struct TengokuSaveData *data, u32 levelID, u8 firstSuperb) {
    if (levelID >= TOTAL_BASE_LEVELS){
        data->extraData.extraLevelFirstSuperb[levelID - TOTAL_BASE_LEVELS] = firstSuperb;
    } else {
        data->levelFirstSuperb[levelID] = firstSuperb;
    }
}

void set_campaign_cleared(struct TengokuSaveData *data, u32 campaignID, u8 cleared) {
    if (campaignID >= TOTAL_BASE_PERFECT_CAMPAIGNS){
        data->extraData.extraCampaignsCleared[campaignID - TOTAL_BASE_PERFECT_CAMPAIGNS] = cleared;
    } else {
        data->campaignsCleared[campaignID] = cleared;
    }
}

void set_reading_material_unlocked(struct TengokuSaveData *data, u32 materialID, u8 unlocked) {
    if (materialID >= TOTAL_BASE_READING_MATERIALS) {
        data->extraData.extraReadingMaterialUnlocked[materialID - TOTAL_BASE_READING_MATERIALS] = unlocked;
    } else {
        data->readingMaterialUnlocked[materialID] = unlocked;
    }
}