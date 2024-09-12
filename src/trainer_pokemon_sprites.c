#include "global.h"
#include "sprite.h"
#include "window.h"
#include "malloc.h"
#include "palette.h"
#include "decompress.h"
#include "trainer_pokemon_sprites.h"
#include "data.h"
#include "pokemon.h"
#include "constants/trainers.h"

#define PICS_COUNT 8

// Needs to be large enough to store either a decompressed Pokémon pic or trainer pic
#define PIC_SPRITE_SIZE max(MON_PIC_SIZE, TRAINER_PIC_SIZE)
#define MAX_PIC_FRAMES  max(MAX_MON_PIC_FRAMES, MAX_TRAINER_PIC_FRAMES)

struct PicData
{
    u8 *frames;
    struct SpriteFrameImage *images;
    u16 paletteTag;
    u8 spriteId;
    u8 active;
};

static EWRAM_DATA struct SpriteTemplate sCreatingSpriteTemplate = {};
static EWRAM_DATA struct PicData sSpritePics[PICS_COUNT] = {};

static const struct PicData sDummyPicData = {};

static const struct OamData sOamData_Normal =
{
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64)
};

static const struct OamData sOamData_Affine =
{
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64)
};

static void DummyPicSpriteCallback(struct Sprite *sprite)
{

}

bool16 ResetAllPicSprites(void)
{
    int i;

    for (i = 0; i < PICS_COUNT; i ++)
        sSpritePics[i] = sDummyPicData;

    return FALSE;
}

static bool16 DecompressPic(u16 species, u32 personality, bool8 isFrontPic, u8 *dest, bool8 isTrainer, bool8 ignoreDeoxys, u8 phenotype)
{
    if (!isTrainer)
    {
        if (isFrontPic)
        {
            if (!ignoreDeoxys)
                LoadSpecialPokePic(dest, species, personality, isFrontPic, phenotype);
            else
                LoadSpecialPokePic_DontHandleDeoxys(dest, species, personality, isFrontPic, phenotype);
        }
        else
        {
            if (!ignoreDeoxys)
                LoadSpecialPokePic(dest, species, personality, isFrontPic, phenotype);
            else
                LoadSpecialPokePic_DontHandleDeoxys(dest, species, personality, isFrontPic, phenotype);
        }
    }
    else
    {
        if (isFrontPic)
            DecompressPicFromTable(&gTrainerFrontPicTable[species], dest, species);
        else
            DecompressPicFromTable(&gTrainerBackPicTable[species], dest, species);
    }
    return FALSE;
}

static bool16 DecompressPic_HandleDeoxys(u16 species, u32 personality, bool8 isFrontPic, u8 *dest, bool8 isTrainer, u8 phenotype)
{
    return DecompressPic(species, personality, isFrontPic, dest, isTrainer, FALSE, phenotype);
}

static void LoadPicPaletteByTagOrSlot(u16 species, u32 otId, u32 personality, u8 paletteSlot, u16 paletteTag, bool8 isTrainer, u8 phenotype)
{
    struct CompressedSpritePalette tempPal;
    if (!isTrainer)
    {
        if (paletteTag == TAG_NONE)
        {
            sCreatingSpriteTemplate.paletteTag = TAG_NONE;
            LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, otId, personality, phenotype), OBJ_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
        }
        else
        {
            sCreatingSpriteTemplate.paletteTag = paletteTag;
            tempPal = GetMonSpritePalStructFromOtIdPersonality(species, otId, personality, phenotype);
            LoadCompressedSpritePalette(&tempPal);
        }
    }
    else
    {
        if (paletteTag == TAG_NONE)
        {
            sCreatingSpriteTemplate.paletteTag = TAG_NONE;
            LoadCompressedPalette(gTrainerFrontPicPaletteTable[species].data, OBJ_PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
        }
        else
        {
            sCreatingSpriteTemplate.paletteTag = paletteTag;
            LoadCompressedSpritePalette(&gTrainerFrontPicPaletteTable[species]);
        }
    }
}

static void LoadPicPaletteBySlot(u16 species, u32 otId, u32 personality, u8 paletteSlot, bool8 isTrainer, u8 phenotype)
{
    if (!isTrainer)
        LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, otId, personality, phenotype), PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
    else
        LoadCompressedPalette(gTrainerFrontPicPaletteTable[species].data, PLTT_ID(paletteSlot), PLTT_SIZE_4BPP);
}

static void AssignSpriteAnimsTable(bool8 isTrainer)
{
    if (!isTrainer)
        sCreatingSpriteTemplate.anims = gAnims_MonPic;
    else
        sCreatingSpriteTemplate.anims = gTrainerFrontAnimsPtrTable[0];
}

static u16 CreatePicSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, bool8 isTrainer, bool8 ignoreDeoxys, u8 phenotype)
{
    u8 i;
    u8 *framePics;
    struct SpriteFrameImage *images;
    int j;
    u8 spriteId;

    for (i = 0; i < PICS_COUNT; i ++)
    {
        if (!sSpritePics[i].active)
            break;
    }
    if (i == PICS_COUNT)
        return 0xFFFF;

    framePics = Alloc(PIC_SPRITE_SIZE * MAX_PIC_FRAMES);
    if (!framePics)
        return 0xFFFF;

    images = Alloc(sizeof(struct SpriteFrameImage) * MAX_PIC_FRAMES);
    if (!images)
    {
        Free(framePics);
        return 0xFFFF;
    }
    if (DecompressPic(species, personality, isFrontPic, framePics, isTrainer, ignoreDeoxys, phenotype))
    {
        // debug trap?
        return 0xFFFF;
    }
    for (j = 0; j < MAX_PIC_FRAMES; j ++)
    {
        images[j].data = framePics + PIC_SPRITE_SIZE * j;
        images[j].size = PIC_SPRITE_SIZE;
    }
    sCreatingSpriteTemplate.tileTag = TAG_NONE;
    sCreatingSpriteTemplate.oam = &sOamData_Normal;
    AssignSpriteAnimsTable(isTrainer);
    sCreatingSpriteTemplate.images = images;
    sCreatingSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    sCreatingSpriteTemplate.callback = DummyPicSpriteCallback;
    LoadPicPaletteByTagOrSlot(species, otId, personality, paletteSlot, paletteTag, isTrainer, phenotype);
    spriteId = CreateSprite(&sCreatingSpriteTemplate, x, y, 0);
    if (paletteTag == TAG_NONE)
        gSprites[spriteId].oam.paletteNum = paletteSlot;
    sSpritePics[i].frames = framePics;
    sSpritePics[i].images = images;
    sSpritePics[i].paletteTag = paletteTag;
    sSpritePics[i].spriteId = spriteId;
    sSpritePics[i].active = TRUE;
    return spriteId;
}

static u16 CreatePicSprite_HandleDeoxys(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, bool8 isTrainer, u8 phenotype)
{
    return CreatePicSprite(species, otId, personality, isFrontPic, x, y, paletteSlot, paletteTag, isTrainer, FALSE, phenotype);
}

u16 CreateMonPicSprite_Affine(u16 species, u32 otId, u32 personality, u8 flags, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, u8 phenotype)
{
    u8 *framePics;
    struct SpriteFrameImage *images;
    int j;
    u8 i;
    u8 spriteId;
    u8 type;

    for (i = 0; i < PICS_COUNT; i++)
    {
        if (!sSpritePics[i].active)
            break;
    }
    if (i == PICS_COUNT)
        return 0xFFFF;

    framePics = Alloc(MON_PIC_SIZE * MAX_MON_PIC_FRAMES);
    if (!framePics)
        return 0xFFFF;

    if (flags & F_MON_PIC_NO_AFFINE)
    {
        flags &= ~F_MON_PIC_NO_AFFINE;
        type = MON_PIC_AFFINE_NONE;
    }
    else
    {
        type = flags;
    }
    images = Alloc(sizeof(struct SpriteFrameImage) * MAX_MON_PIC_FRAMES);
    if (!images)
    {
        Free(framePics);
        return 0xFFFF;
    }
    if (DecompressPic(species, personality, flags, framePics, FALSE, FALSE, phenotype))
    {
        // debug trap?
        return 0xFFFF;
    }
    for (j = 0; j < MAX_MON_PIC_FRAMES; j ++)
    {
        images[j].data = framePics + MON_PIC_SIZE * j;
        images[j].size = MON_PIC_SIZE;
    }
    sCreatingSpriteTemplate.tileTag = TAG_NONE;
    sCreatingSpriteTemplate.anims = gMonFrontAnimsPtrTable[species];
    sCreatingSpriteTemplate.images = images;
    if (type == MON_PIC_AFFINE_FRONT)
    {
        sCreatingSpriteTemplate.affineAnims = gAffineAnims_BattleSpriteOpponentSide;
        sCreatingSpriteTemplate.oam = &sOamData_Affine;
    }
    else if (type == MON_PIC_AFFINE_BACK)
    {
        sCreatingSpriteTemplate.affineAnims = gAffineAnims_BattleSpritePlayerSide;
        sCreatingSpriteTemplate.oam = &sOamData_Affine;
    }
    else // MON_PIC_AFFINE_NONE
    {
        sCreatingSpriteTemplate.oam = &sOamData_Normal;
        sCreatingSpriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    }
    sCreatingSpriteTemplate.callback = DummyPicSpriteCallback;
    LoadPicPaletteByTagOrSlot(species, otId, personality, paletteSlot, paletteTag, FALSE, phenotype);
    spriteId = CreateSprite(&sCreatingSpriteTemplate, x, y, 0);
    if (paletteTag == TAG_NONE)
        gSprites[spriteId].oam.paletteNum = paletteSlot;
    sSpritePics[i].frames = framePics;
    sSpritePics[i].images = images;
    sSpritePics[i].paletteTag = paletteTag;
    sSpritePics[i].spriteId = spriteId;
    sSpritePics[i].active = TRUE;
    return spriteId;
}

static u16 FreeAndDestroyPicSpriteInternal(u16 spriteId)
{
    u8 i;
    u8 *framePics;
    struct SpriteFrameImage *images;

    for (i = 0; i < PICS_COUNT; i ++)
    {
        if (sSpritePics[i].spriteId == spriteId)
            break;
    }
    if (i == PICS_COUNT)
        return 0xFFFF;

    framePics = sSpritePics[i].frames;
    images = sSpritePics[i].images;
    if (sSpritePics[i].paletteTag != TAG_NONE)
        FreeSpritePaletteByTag(GetSpritePaletteTagByPaletteNum(gSprites[spriteId].oam.paletteNum));
    DestroySprite(&gSprites[spriteId]);
    Free(framePics);
    Free(images);
    sSpritePics[i] = sDummyPicData;
    return 0;
}

static u16 LoadPicSpriteInWindow(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u8 paletteSlot, u8 windowId, bool8 isTrainer, u8 phenotype)
{
    if (DecompressPic_HandleDeoxys(species, personality, isFrontPic, (u8 *)GetWindowAttribute(windowId, WINDOW_TILE_DATA), FALSE, phenotype))
        return 0xFFFF;

    LoadPicPaletteBySlot(species, otId, personality, paletteSlot, isTrainer, phenotype);
    return 0;
}

static u16 CreateTrainerCardSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u16 destX, u16 destY, u8 paletteSlot, u8 windowId, bool8 isTrainer, u8 phenotype)
{
    u8 *framePics;

    framePics = Alloc(TRAINER_PIC_SIZE * MAX_TRAINER_PIC_FRAMES);
    if (framePics && !DecompressPic_HandleDeoxys(species, personality, isFrontPic, framePics, isTrainer, phenotype))
    {
        BlitBitmapRectToWindow(windowId, framePics, 0, 0, TRAINER_PIC_WIDTH, TRAINER_PIC_HEIGHT, destX, destY, TRAINER_PIC_WIDTH, TRAINER_PIC_HEIGHT);
        LoadPicPaletteBySlot(species, otId, personality, paletteSlot, isTrainer, phenotype);
        Free(framePics);
        return 0;
    }
    return 0xFFFF;
}

u16 CreateMonPicSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, bool8 ignoreDeoxys, u8 phenotype)
{
    return CreatePicSprite(species, otId, personality, isFrontPic, x, y, paletteSlot, paletteTag, FALSE, ignoreDeoxys, phenotype);
}
u16 CreateMonPicSprite_HandleDeoxys(u16 species, u32 otId, u32 personality, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag, u8 phenotype)
{
    return CreateMonPicSprite(species, otId, personality, isFrontPic, x, y, paletteSlot, paletteTag, FALSE, phenotype);
}

u16 FreeAndDestroyMonPicSprite(u16 spriteId)
{
    return FreeAndDestroyPicSpriteInternal(spriteId);
}

static u16 UNUSED LoadMonPicInWindow(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u8 paletteSlot, u8 windowId, u8 phenotype)
{
    return LoadPicSpriteInWindow(species, otId, personality, isFrontPic, paletteSlot, windowId, FALSE, phenotype);
}

// Unused, FRLG only
u16 CreateTrainerCardMonIconSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic, u16 destX, u16 destY, u8 paletteSlot, u8 windowId, u8 phenotype)
{
    return CreateTrainerCardSprite(species, otId, personality, isFrontPic, destX, destY, paletteSlot, windowId, FALSE, phenotype);
}

u16 CreateTrainerPicSprite(u16 species, bool8 isFrontPic, s16 x, s16 y, u8 paletteSlot, u16 paletteTag)
{
    return CreatePicSprite_HandleDeoxys(species, 0, 0, isFrontPic, x, y, paletteSlot, paletteTag, TRUE, 0);
}

u16 FreeAndDestroyTrainerPicSprite(u16 spriteId)
{
    return FreeAndDestroyPicSpriteInternal(spriteId);
}

static u16 UNUSED LoadTrainerPicInWindow(u16 species, bool8 isFrontPic, u8 paletteSlot, u8 windowId)
{
    return LoadPicSpriteInWindow(species, 0, 0, isFrontPic, paletteSlot, windowId, TRUE, 0);
}

u16 CreateTrainerCardTrainerPicSprite(u16 species, bool8 isFrontPic, u16 destX, u16 destY, u8 paletteSlot, u8 windowId)
{
    return CreateTrainerCardSprite(species, 0, 0, isFrontPic, destX, destY, paletteSlot, windowId, TRUE, 0);
}

u16 PlayerGenderToFrontTrainerPicId_Debug(u8 gender, bool8 getClass)
{
    if (getClass == TRUE)
    {
        if (gender != MALE)
            return gFacilityClassToPicIndex[FACILITY_CLASS_MAY];
        else
            return gFacilityClassToPicIndex[FACILITY_CLASS_BRENDAN];
    }
    return gender;
}
