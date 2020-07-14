﻿#pragma once

  //
 //   VARIABLES   ///////////////////////////////////////////////// ///////  //////   /////    ///     //      /
//

FC_Font* font;
FC_Font* font_under;
FC_Font* font_bold;

TTF_Font* FONT;
uint16_t FONT_CHRW;
uint16_t FONT_CHRH;

bool QUIT = false;
float FPS = 0;

int16_t MOUSE_X;
int16_t MOUSE_Y;
int16_t MOUSE_PREVX;
int16_t MOUSE_PREVY;

int WINDOW_X;
int WINDOW_Y;
int WINDOW_W;
int WINDOW_H;

int DISPLAY_W;
int DISPLAY_H;

struct COLOR {
	uint8_t r, g, b, a;

	friend bool operator==(COLOR a, COLOR b) {
		return a.r == b.r && a.g == b.g
			&& a.b == b.b && a.a == b.a;
	}

	friend bool operator!=(COLOR a, COLOR b) {
		return !(a == b);
	}
};

// CANVAS
bool CANVAS_UPDATE = false;
float CANVAS_ZOOM = 1.0;
float CANVAS_X = 0.0;
float CANVAS_Y = 0.0;
uint16_t CANVAS_W = 512;
uint16_t CANVAS_H = 256;
int16_t CANVAS_MOUSE_X = 0;
int16_t CANVAS_MOUSE_Y = 0;
int16_t CANVAS_MOUSE_CLICKX = 0;
int16_t CANVAS_MOUSE_CLICKY = 0;
int16_t CANVAS_MOUSE_CLICKPREVX = 0;
int16_t CANVAS_MOUSE_CLICKPREVY = 0;
int16_t CANVAS_MOUSE_PREVX = 0;
int16_t CANVAS_MOUSE_PREVY = 0;
int16_t CANVAS_MOUSE_CELL_X = 0;
int16_t CANVAS_MOUSE_CELL_Y = 0;
int16_t CANVAS_PITCH = (sizeof(COLOR) * CANVAS_W);

SDL_Texture* BG_GRID_TEXTURE;
int16_t CELL_W = 16;
int16_t CELL_H = 16;
int16_t BG_GRID_W = 0;
int16_t BG_GRID_H = 0;

float CANVAS_PANX = 0.0;
float CANVAS_PANY = 0.0;

float CANVAS_X_ANIM = 0.0;
float CANVAS_Y_ANIM = 0.0;
float CANVAS_W_ANIM = 0.0;
float CANVAS_H_ANIM = 0.0;
float CELL_W_ANIM = 0.0;
float CELL_H_ANIM = 0.0;

// LAYER
uint16_t CURRENT_LAYER = 0;
COLOR* CURRENT_LAYER_PTR = nullptr;
int16_t LAYER_UPDATE = 0;
int16_t LAYER_UPDATE_X1 = INT16_MAX;
int16_t LAYER_UPDATE_Y1 = INT16_MAX;
int16_t LAYER_UPDATE_X2 = INT16_MIN;
int16_t LAYER_UPDATE_Y2 = INT16_MIN;
struct LAYER_INFO {
	SDL_Texture* texture;
	std::unique_ptr<COLOR[]> pixels;
	int16_t x;
	int16_t y;
	uint8_t alpha;
	SDL_BlendMode blendmode;
};
std::vector<LAYER_INFO> LAYERS;

// BRUSH
SDL_Texture* BRUSH_CURSOR_TEXTURE;
SDL_Texture* BRUSH_TEXTURE;
bool BRUSH_UPDATE = 0;

int16_t BRUSH_UPDATE_X1 = INT16_MAX;
int16_t BRUSH_UPDATE_Y1 = INT16_MAX;
int16_t BRUSH_UPDATE_X2 = INT16_MIN;
int16_t BRUSH_UPDATE_Y2 = INT16_MIN;
std::unique_ptr<COLOR[]> BRUSH_PIXELS;
COLOR BRUSH_COLOR {255, 64, 128, 64};
COLOR* BRUSH_CURSOR_PIXELS;
COLOR* BRUSH_CURSOR_PIXELS_CLEAR;
SDL_Rect BRUSH_CURSOR_PIXELS_CLEAR_RECT;
uint16_t BRUSH_CURSOR_PIXELS_CLEAR_POS;

//SDL_Texture* CANVAS_TEXTURE;// = SDL_CreateTexture(RENDERER, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, CANVAS_W, CANVAS_H);
//uint32_t* CANVAS_PIXELS;// = new uint32_t[CANVAS_W * CANVAS_H];

bool MOUSEBUTTON_LEFT = false;
bool MOUSEBUTTON_PRESSED_LEFT = false;
bool MOUSEBUTTON_MIDDLE = false;
bool MOUSEBUTTON_PRESSED_MIDDLE = false;
bool MOUSEBUTTON_RIGHT = false;
bool MOUSEBUTTON_PRESSED_RIGHT = false;

// TOOL
uint16_t CURRENT_TOOL = 0;

// UI
int16_t UIBOX_IN = -1;
int16_t UIBOX_CLICKED_IN = -1;
int16_t UIBOX_PANX = 0;
int16_t UIBOX_PANY = 0;
SDL_Texture* UI_TEXTURE_HUEBAR;
COLOR* UI_PIXELS_HUEBAR;

struct UIBOX_CHARINFO {
	const char* chr;
	bool update;
	COLOR col;
};

struct UIBOX_INFO {
	bool update = 1;
	std::vector<UIBOX_CHARINFO> charinfo;
	SDL_Texture* texture;
	uint16_t tex_w;
	uint16_t tex_h;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	uint8_t alpha;
};
std::vector<UIBOX_INFO> UIBOXES;

// UNDO
struct UNDO_DATA
{
	uint16_t layer = 0;
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t w;
	uint16_t h;
	std::vector<COLOR> undo_pixels;
	std::vector<COLOR> redo_pixels;

	UNDO_DATA(uint16_t _w, uint16_t _h)
		: w {_w}
		, h {_h}
		, undo_pixels(_w*_h)
		, redo_pixels(_w*_h)
	{
	}

	void set(uint16_t xx, uint16_t yy, COLOR prev_col, COLOR new_col)
	{
		int index = xx + yy * this->w;
		undo_pixels[index] = prev_col;
		redo_pixels[index] = new_col;
	}
};

std::vector<UNDO_DATA> UNDO_LIST;
uint16_t UNDO_POS = 0;
uint16_t UNDO_UPDATE = 0;
uint16_t UNDO_UPDATE_LAYER = 0;
SDL_Rect UNDO_UPDATE_RECT = { 0, 0, 1, 1 };
COLOR UNDO_COL {0xff, 0x00, 0x40, 0xc0};

static const bool arrow[] = {
	1,0,0,0,0,0,1,0,
	0,1,0,0,0,1,0,0,
	0,0,1,0,1,0,0,0,
	0,0,0,1,0,0,0,0,
	0,0,1,0,1,0,0,0,
	0,1,0,0,0,1,0,0,
	1,0,0,0,0,0,1,0,
	0,0,0,0,0,0,0,0
};

static SDL_Cursor* init_system_cursor(const bool image[])
{
	Uint8 data[8*8];
	Uint8 mask[8*8];
	int n = -1;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (j % 8) {
				data[n] <<= 1;
				mask[n] <<= 1;
			}
			else {
				++n;
				data[n] = mask[i] = 0;
			}
			if (arrow[i * 8 + j])
			{
				data[n] |= 0x00;
				mask[n] |= 0x01;
			}
		}
	}
	return SDL_CreateCursor(data, mask, 8, 8, 3, 3);
}

const char* CHAR_BOXTL = u8"╔";
const char* CHAR_BOXTR = u8"╗";
const char* CHAR_BOXBL = u8"╚";
const char* CHAR_BOXBR = u8"╝";
const char* CHAR_BOXH = u8"═";
const char* CHAR_BOXV = u8"║";