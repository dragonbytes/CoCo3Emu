#pragma once

constexpr uint16_t vdgLowercaseOffset = (64 * 12);
// Font bitmaps all come from MAME source code

//-------------------------------------------------
//  semigraphics4_fontdata8x12
//-------------------------------------------------
uint8_t sg4_fontdata8x12[192] =
{
	/* Block Graphics (Semigraphics 4 Graphics ) */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

//-------------------------------------------------
//  lowres_font
//-------------------------------------------------
uint8_t lowres_font[1152] =
{
	0x00, 0x38, 0x44, 0x04, 0x34, 0x4C, 0x4C, 0x38, 0x00, 0x00, 0x00, 0x00, /* @ */
	0x00, 0x10, 0x28, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, /* A */
	0x00, 0x78, 0x24, 0x24, 0x38, 0x24, 0x24, 0x78, 0x00, 0x00, 0x00, 0x00, /* B */
	0x00, 0x38, 0x44, 0x40, 0x40, 0x40, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* C */
	0x00, 0x78, 0x24, 0x24, 0x24, 0x24, 0x24, 0x78, 0x00, 0x00, 0x00, 0x00, /* D */
	0x00, 0x7C, 0x40, 0x40, 0x70, 0x40, 0x40, 0x7C, 0x00, 0x00, 0x00, 0x00, /* E */
	0x00, 0x7C, 0x40, 0x40, 0x70, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, /* F */
	0x00, 0x38, 0x44, 0x40, 0x40, 0x4C, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* G */
	0x00, 0x44, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, /* H */
	0x00, 0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x00, /* I */
	0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* J */
	0x00, 0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 0x44, 0x00, 0x00, 0x00, 0x00, /* K */
	0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7C, 0x00, 0x00, 0x00, 0x00, /* L */
	0x00, 0x44, 0x6C, 0x54, 0x54, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, /* M */
	0x00, 0x44, 0x44, 0x64, 0x54, 0x4C, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, /* N */
	0x00, 0x38, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* O */
	0x00, 0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, /* P */
	0x00, 0x38, 0x44, 0x44, 0x44, 0x54, 0x48, 0x34, 0x00, 0x00, 0x00, 0x00, /* Q */
	0x00, 0x78, 0x44, 0x44, 0x78, 0x50, 0x48, 0x44, 0x00, 0x00, 0x00, 0x00, /* R */
	0x00, 0x38, 0x44, 0x40, 0x38, 0x04, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* S */
	0x00, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, /* T */
	0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* U */
	0x00, 0x44, 0x44, 0x44, 0x28, 0x28, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, /* V */
	0x00, 0x44, 0x44, 0x44, 0x44, 0x54, 0x6C, 0x44, 0x00, 0x00, 0x00, 0x00, /* W */
	0x00, 0x44, 0x44, 0x28, 0x10, 0x28, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, /* X */
	0x00, 0x44, 0x44, 0x28, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, /* Y */
	0x00, 0x7C, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7C, 0x00, 0x00, 0x00, 0x00, /* Z */
	0x00, 0x38, 0x20, 0x20, 0x20, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00, 0x00, /* [ */
	0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, /* \ */
	0x00, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x38, 0x00, 0x00, 0x00, 0x00, /* ] */
	0x00, 0x10, 0x38, 0x54, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, /* up arrow */
	0x00, 0x00, 0x10, 0x20, 0x7C, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, /* left arrow */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* space */
	0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, /* ! */
	0x00, 0x28, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* " */
	0x00, 0x28, 0x28, 0x7C, 0x28, 0x7C, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, /* # */
	0x00, 0x10, 0x3C, 0x50, 0x38, 0x14, 0x78, 0x10, 0x00, 0x00, 0x00, 0x00, /* $ */
	0x00, 0x60, 0x64, 0x08, 0x10, 0x20, 0x4C, 0x0C, 0x00, 0x00, 0x00, 0x00, /* % */
	0x00, 0x20, 0x50, 0x50, 0x20, 0x54, 0x48, 0x34, 0x00, 0x00, 0x00, 0x00, /* & */
	0x00, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' */
	0x00, 0x08, 0x10, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, /* ( */
	0x00, 0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, /* ) */
	0x00, 0x00, 0x10, 0x54, 0x38, 0x38, 0x54, 0x10, 0x00, 0x00, 0x00, 0x00, /* * */
	0x00, 0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, /* + */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, /* , */
	0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* - */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, /* . */
	0x00, 0x00, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, /* / */
	0x00, 0x38, 0x44, 0x4C, 0x54, 0x64, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* 0 */
	0x00, 0x10, 0x30, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x00, /* 1 */
	0x00, 0x38, 0x44, 0x04, 0x38, 0x40, 0x40, 0x7C, 0x00, 0x00, 0x00, 0x00, /* 2 */
	0x00, 0x38, 0x44, 0x04, 0x08, 0x04, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* 3 */
	0x00, 0x08, 0x18, 0x28, 0x48, 0x7C, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, /* 4 */
	0x00, 0x7C, 0x40, 0x78, 0x04, 0x04, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* 5 */
	0x00, 0x38, 0x40, 0x40, 0x78, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* 6 */
	0x00, 0x7C, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, /* 7 */
	0x00, 0x38, 0x44, 0x44, 0x38, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* 8 */
	0x00, 0x38, 0x44, 0x44, 0x38, 0x04, 0x04, 0x38, 0x00, 0x00, 0x00, 0x00, /* 9 */
	0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, /* : */
	0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, /* ; */
	0x00, 0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, /* < */
	0x00, 0x00, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* = */
	0x00, 0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, /* > */
	0x00, 0x38, 0x44, 0x04, 0x08, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, /* ? */

	/* Lower case */
	0x00, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ^ */
	0x00, 0x00, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00, 0x00, 0x00, 0x00, /* a */
	0x00, 0x40, 0x40, 0x58, 0x64, 0x44, 0x64, 0x58, 0x00, 0x00, 0x00, 0x00, /* b */
	0x00, 0x00, 0x00, 0x38, 0x44, 0x40, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* c */
	0x00, 0x04, 0x04, 0x34, 0x4C, 0x44, 0x4C, 0x34, 0x00, 0x00, 0x00, 0x00, /* d */
	0x00, 0x00, 0x00, 0x38, 0x44, 0x7C, 0x40, 0x38, 0x00, 0x00, 0x00, 0x00, /* e */
	0x00, 0x08, 0x14, 0x10, 0x38, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, /* f */
	0x00, 0x00, 0x00, 0x34, 0x4C, 0x4C, 0x34, 0x04, 0x38, 0x00, 0x00, 0x00, /* g */
	0x00, 0x40, 0x40, 0x58, 0x64, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, /* h */
	0x00, 0x00, 0x10, 0x00, 0x30, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x00, /* i */
	0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x04, 0x44, 0x38, 0x00, 0x00, 0x00, /* j */
	0x00, 0x40, 0x40, 0x48, 0x50, 0x60, 0x50, 0x48, 0x00, 0x00, 0x00, 0x00, /* k */
	0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x00, /* l */
	0x00, 0x00, 0x00, 0x68, 0x54, 0x54, 0x54, 0x54, 0x00, 0x00, 0x00, 0x00, /* m */
	0x00, 0x00, 0x00, 0x58, 0x64, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, /* n */
	0x00, 0x00, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, /* o */
	0x00, 0x00, 0x00, 0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x00, 0x00, 0x00, /* p */
	0x00, 0x00, 0x00, 0x3C, 0x44, 0x44, 0x3C, 0x04, 0x04, 0x00, 0x00, 0x00, /* q */
	0x00, 0x00, 0x00, 0x58, 0x64, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, /* r */
	0x00, 0x00, 0x00, 0x3C, 0x40, 0x38, 0x04, 0x78, 0x00, 0x00, 0x00, 0x00, /* s */
	0x00, 0x20, 0x20, 0x70, 0x20, 0x20, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00, /* t */
	0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x4C, 0x34, 0x00, 0x00, 0x00, 0x00, /* u */
	0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x28, 0x10, 0x00, 0x00, 0x00, 0x00, /* v */
	0x00, 0x00, 0x00, 0x44, 0x54, 0x54, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, /* w */
	0x00, 0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, /* x */
	0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x3C, 0x04, 0x38, 0x00, 0x00, 0x00, /* y */
	0x00, 0x00, 0x00, 0x7C, 0x08, 0x10, 0x20, 0x7C, 0x00, 0x00, 0x00, 0x00, /* z */
	0x00, 0x08, 0x10, 0x10, 0x20, 0x10, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, /* { */
	0x00, 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, /* | */
	0x00, 0x20, 0x10, 0x10, 0x08, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, /* } */
	0x00, 0x20, 0x54, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ~ */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00  /* _ */
};

//-------------------------------------------------
//  hires_font
//-------------------------------------------------
uint8_t hires_font[128][8] =
{
	{ 0x38, 0x44, 0x40, 0x40, 0x40, 0x44, 0x38, 0x10}, { 0x44, 0x00, 0x44, 0x44, 0x44, 0x4C, 0x34, 0x00},
	{ 0x08, 0x10, 0x38, 0x44, 0x7C, 0x40, 0x38, 0x00}, { 0x10, 0x28, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00},
	{ 0x28, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00}, { 0x20, 0x10, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00},
	{ 0x10, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00}, { 0x00, 0x00, 0x38, 0x44, 0x40, 0x44, 0x38, 0x10},
	{ 0x10, 0x28, 0x38, 0x44, 0x7C, 0x40, 0x38, 0x00}, { 0x28, 0x00, 0x38, 0x44, 0x7C, 0x40, 0x38, 0x00},
	{ 0x20, 0x10, 0x38, 0x44, 0x7C, 0x40, 0x38, 0x00}, { 0x28, 0x00, 0x30, 0x10, 0x10, 0x10, 0x38, 0x00},
	{ 0x10, 0x28, 0x00, 0x30, 0x10, 0x10, 0x38, 0x00}, { 0x00, 0x18, 0x24, 0x38, 0x24, 0x24, 0x38, 0x40},
	{ 0x44, 0x10, 0x28, 0x44, 0x7C, 0x44, 0x44, 0x00}, { 0x10, 0x10, 0x28, 0x44, 0x7C, 0x44, 0x44, 0x00},
	{ 0x08, 0x10, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00}, { 0x00, 0x00, 0x68, 0x14, 0x3C, 0x50, 0x3C, 0x00},
	{ 0x3C, 0x50, 0x50, 0x78, 0x50, 0x50, 0x5C, 0x00}, { 0x10, 0x28, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00},
	{ 0x28, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00}, { 0x00, 0x00, 0x38, 0x4C, 0x54, 0x64, 0x38, 0x00},
	{ 0x10, 0x28, 0x00, 0x44, 0x44, 0x4C, 0x34, 0x00}, { 0x20, 0x10, 0x44, 0x44, 0x44, 0x4C, 0x34, 0x00},
	{ 0x38, 0x4C, 0x54, 0x54, 0x54, 0x64, 0x38, 0x00}, { 0x44, 0x38, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00},
	{ 0x28, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00}, { 0x38, 0x40, 0x38, 0x44, 0x38, 0x04, 0x38, 0x00},
	{ 0x08, 0x14, 0x10, 0x38, 0x10, 0x14, 0x78, 0x00}, { 0x10, 0x10, 0x7C, 0x10, 0x10, 0x00, 0x7C, 0x00},
	{ 0x10, 0x28, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00}, { 0x08, 0x14, 0x10, 0x38, 0x10, 0x10, 0x50, 0x20},
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, { 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00},
	{ 0x28, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00}, { 0x28, 0x28, 0x7C, 0x28, 0x7C, 0x28, 0x28, 0x00},
	{ 0x10, 0x3C, 0x50, 0x38, 0x14, 0x78, 0x10, 0x00}, { 0x60, 0x64, 0x08, 0x10, 0x20, 0x4C, 0x0C, 0x00},
	{ 0x20, 0x50, 0x50, 0x20, 0x54, 0x48, 0x34, 0x00}, { 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},
	{ 0x08, 0x10, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00}, { 0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00},
	{ 0x00, 0x10, 0x54, 0x38, 0x38, 0x54, 0x10, 0x00}, { 0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x00, 0x00},
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20}, { 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00}, { 0x00, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00},
	{ 0x38, 0x44, 0x4C, 0x54, 0x64, 0x44, 0x38, 0x00}, { 0x10, 0x30, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00},
	{ 0x38, 0x44, 0x04, 0x38, 0x40, 0x40, 0x7C, 0x00}, { 0x38, 0x44, 0x04, 0x08, 0x04, 0x44, 0x38, 0x00},
	{ 0x08, 0x18, 0x28, 0x48, 0x7C, 0x08, 0x08, 0x00}, { 0x7C, 0x40, 0x78, 0x04, 0x04, 0x44, 0x38, 0x00},
	{ 0x38, 0x40, 0x40, 0x78, 0x44, 0x44, 0x38, 0x00}, { 0x7C, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40, 0x00},
	{ 0x38, 0x44, 0x44, 0x38, 0x44, 0x44, 0x38, 0x00}, { 0x38, 0x44, 0x44, 0x38, 0x04, 0x04, 0x38, 0x00},
	{ 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00}, { 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x10, 0x20},
	{ 0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08, 0x00}, { 0x00, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x00, 0x00},
	{ 0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20, 0x00}, { 0x38, 0x44, 0x04, 0x08, 0x10, 0x00, 0x10, 0x00},
	{ 0x38, 0x44, 0x04, 0x34, 0x4C, 0x4C, 0x38, 0x00}, { 0x10, 0x28, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x00},
	{ 0x78, 0x24, 0x24, 0x38, 0x24, 0x24, 0x78, 0x00}, { 0x38, 0x44, 0x40, 0x40, 0x40, 0x44, 0x38, 0x00},
	{ 0x78, 0x24, 0x24, 0x24, 0x24, 0x24, 0x78, 0x00}, { 0x7C, 0x40, 0x40, 0x70, 0x40, 0x40, 0x7C, 0x00},
	{ 0x7C, 0x40, 0x40, 0x70, 0x40, 0x40, 0x40, 0x00}, { 0x38, 0x44, 0x40, 0x40, 0x4C, 0x44, 0x38, 0x00},
	{ 0x44, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44, 0x00}, { 0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00},
	{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x44, 0x38, 0x00}, { 0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 0x44, 0x00},
	{ 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7C, 0x00}, { 0x44, 0x6C, 0x54, 0x54, 0x44, 0x44, 0x44, 0x00},
	{ 0x44, 0x44, 0x64, 0x54, 0x4C, 0x44, 0x44, 0x00}, { 0x38, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00},
	{ 0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x40, 0x00}, { 0x38, 0x44, 0x44, 0x44, 0x54, 0x48, 0x34, 0x00},
	{ 0x78, 0x44, 0x44, 0x78, 0x50, 0x48, 0x44, 0x00}, { 0x38, 0x44, 0x40, 0x38, 0x04, 0x44, 0x38, 0x00},
	{ 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00}, { 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00},
	{ 0x44, 0x44, 0x44, 0x28, 0x28, 0x10, 0x10, 0x00}, { 0x44, 0x44, 0x44, 0x44, 0x54, 0x6C, 0x44, 0x00},
	{ 0x44, 0x44, 0x28, 0x10, 0x28, 0x44, 0x44, 0x00}, { 0x44, 0x44, 0x28, 0x10, 0x10, 0x10, 0x10, 0x00},
	{ 0x7C, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7C, 0x00}, { 0x38, 0x20, 0x20, 0x20, 0x20, 0x20, 0x38, 0x00},
	{ 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00}, { 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x38, 0x00},
	{ 0x10, 0x38, 0x54, 0x10, 0x10, 0x10, 0x10, 0x00}, { 0x00, 0x10, 0x20, 0x7C, 0x20, 0x10, 0x00, 0x00},
	{ 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00}, { 0x00, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00},
	{ 0x40, 0x40, 0x58, 0x64, 0x44, 0x64, 0x58, 0x00}, { 0x00, 0x00, 0x38, 0x44, 0x40, 0x44, 0x38, 0x00},
	{ 0x04, 0x04, 0x34, 0x4C, 0x44, 0x4C, 0x34, 0x00}, { 0x00, 0x00, 0x38, 0x44, 0x7C, 0x40, 0x38, 0x00},
	{ 0x08, 0x14, 0x10, 0x38, 0x10, 0x10, 0x10, 0x00}, { 0x00, 0x00, 0x34, 0x4C, 0x4C, 0x34, 0x04, 0x38},
	{ 0x40, 0x40, 0x58, 0x64, 0x44, 0x44, 0x44, 0x00}, { 0x00, 0x10, 0x00, 0x30, 0x10, 0x10, 0x38, 0x00},
	{ 0x00, 0x04, 0x00, 0x04, 0x04, 0x04, 0x44, 0x38}, { 0x40, 0x40, 0x48, 0x50, 0x60, 0x50, 0x48, 0x00},
	{ 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00}, { 0x00, 0x00, 0x68, 0x54, 0x54, 0x54, 0x54, 0x00},
	{ 0x00, 0x00, 0x58, 0x64, 0x44, 0x44, 0x44, 0x00}, { 0x00, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00},
	{ 0x00, 0x00, 0x78, 0x44, 0x44, 0x78, 0x40, 0x40}, { 0x00, 0x00, 0x3C, 0x44, 0x44, 0x3C, 0x04, 0x04},
	{ 0x00, 0x00, 0x58, 0x64, 0x40, 0x40, 0x40, 0x00}, { 0x00, 0x00, 0x3C, 0x40, 0x38, 0x04, 0x78, 0x00},
	{ 0x20, 0x20, 0x70, 0x20, 0x20, 0x24, 0x18, 0x00}, { 0x00, 0x00, 0x44, 0x44, 0x44, 0x4C, 0x34, 0x00},
	{ 0x00, 0x00, 0x44, 0x44, 0x44, 0x28, 0x10, 0x00}, { 0x00, 0x00, 0x44, 0x54, 0x54, 0x28, 0x28, 0x00},
	{ 0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00}, { 0x00, 0x00, 0x44, 0x44, 0x44, 0x3C, 0x04, 0x38},
	{ 0x00, 0x00, 0x7C, 0x08, 0x10, 0x20, 0x7C, 0x00}, { 0x08, 0x10, 0x10, 0x20, 0x10, 0x10, 0x08, 0x00},
	{ 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x00}, { 0x20, 0x10, 0x10, 0x08, 0x10, 0x10, 0x20, 0x00},
	{ 0x20, 0x54, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00}
};