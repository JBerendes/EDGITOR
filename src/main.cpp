#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <memory>

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_ttf.h>
#endif

#include "SDL_FontCache.h"
#include "VARIABLES.h"
#include "FUNCTIONS.h"

  //
 //   MAIN LOOP   ///////////////////////////////////////////////// ///////  //////   /////    ////     ///      //       /
//

int main(int, char*[])
{
#if __APPLE__
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
    
	// MAIN INIT
	INIT_SDL();
	auto WINDOW = INIT_WINDOW();
	auto RENDERER = INIT_RENDERER(WINDOW);
	auto FONTMAP = INIT_FONT(RENDERER);
	SDL_SetTextureBlendMode(FONTMAP, SDL_BLENDMODE_NONE);

	while (!QUIT) // MAIN LOOP
	{
        const char *error = SDL_GetError();
        if (*error) {
            //QUIT = 1;
            std::cout << "SDL Error: " << error << std::endl;
            SDL_ClearError();
            //break;
        }
        
        
		const Uint64 fps_start = SDL_GetPerformanceCounter(); // fps counter

		BRUSH_UPDATE = 0; // reset brush update

		float t_win_w = (float)WINDOW_W, t_win_h = (float)WINDOW_H; // temporary window size

		// SET WINDOW X, Y, W, H
		// CLEAR RENDER TARGET
		SDL_GetWindowSize(WINDOW, &WINDOW_W, &WINDOW_H);
		SDL_GetWindowPosition(WINDOW, &WINDOW_X, &WINDOW_Y);
		SDL_SetRenderTarget(RENDERER, nullptr);
		SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 255);
		SDL_RenderClear(RENDERER);

		// recenter the canvas if the window changes size
		if ((WINDOW_W != t_win_w) || (WINDOW_H != t_win_h))
		{
			CANVAS_X = (((float)WINDOW_W * .5f) - ((t_win_w * .5f) - CANVAS_X));
			CANVAS_Y = (((float)WINDOW_H * .5f) - ((t_win_h * .5f) - CANVAS_Y));
		}

		///////////////////////////////////////////////// ///////  //////   /////    ////     ///      //       /
		EVENT_LOOP();
		///////////////////////////////////////////////// ///////  //////   /////    ////     ///      //       /

		// UPDATE THE BRUSH TEXTURE PER-CHANGE
		// this is because a complex shape might be drawn in one tick; like floodfill
		if (BRUSH_UPDATE) {
			// make sure the brush_update isn't beyond the canvas
			BRUSH_UPDATE_X1 = (clamp(BRUSH_UPDATE_X1, 0, CANVAS_W));
			BRUSH_UPDATE_Y1 = (clamp(BRUSH_UPDATE_Y1, 0, CANVAS_H));
			BRUSH_UPDATE_X2 = (clamp(BRUSH_UPDATE_X2, 0, CANVAS_W));
			BRUSH_UPDATE_Y2 = (clamp(BRUSH_UPDATE_Y2, 0, CANVAS_H));

			SDL_Rect const brush_dirty_rect {
				BRUSH_UPDATE_X1, BRUSH_UPDATE_Y1,
				(BRUSH_UPDATE_X2 - BRUSH_UPDATE_X1), (BRUSH_UPDATE_Y2 - BRUSH_UPDATE_Y1),
			};

			// update the brush texture
			SDL_SetTextureBlendMode(BRUSH_TEXTURE, SDL_BLENDMODE_NONE);
			SDL_UpdateTexture(BRUSH_TEXTURE, &brush_dirty_rect, &BRUSH_PIXELS[BRUSH_UPDATE_Y1 * CANVAS_W + BRUSH_UPDATE_X1], CANVAS_PITCH);

			// the layer updates only when we stop drawing - for performance.
			// so we constantly update the min and max bounds
			LAYER_UPDATE_X1 = std::min(LAYER_UPDATE_X1, BRUSH_UPDATE_X1);
			LAYER_UPDATE_Y1 = std::min(LAYER_UPDATE_Y1, BRUSH_UPDATE_Y1);
			LAYER_UPDATE_X2 = std::max(LAYER_UPDATE_X2, BRUSH_UPDATE_X2);
			LAYER_UPDATE_Y2 = std::max(LAYER_UPDATE_Y2, BRUSH_UPDATE_Y2);

			// reset the brush bounds with every tick
			BRUSH_UPDATE_X1 = INT16_MAX;
			BRUSH_UPDATE_Y1 = INT16_MAX;
			BRUSH_UPDATE_X2 = INT16_MIN;
			BRUSH_UPDATE_Y2 = INT16_MIN;
		}

		// LAYER UPDATE
		int t_layer_update_w = std::max((int)LAYER_UPDATE_X2 - (int)LAYER_UPDATE_X1, 0), t_layer_update_h = std::max((int)LAYER_UPDATE_Y2 - (int)LAYER_UPDATE_Y1, 0); // probably don't need these max()

		if ((LAYER_UPDATE == 1) && (t_layer_update_w > 0) && (t_layer_update_h > 0))
		{
			UNDO_DATA _u {(uint16_t)t_layer_update_w, (uint16_t)t_layer_update_h};
			_u.x = (uint16_t)LAYER_UPDATE_X1;
			_u.y = (uint16_t)LAYER_UPDATE_Y1;
			_u.layer = CURRENT_LAYER;

			COLOR* layer_data = (LAYERS[CURRENT_LAYER].pixels.get());
			for (int16_t _y = LAYER_UPDATE_Y1; _y < LAYER_UPDATE_Y2; ++_y) {
				for (int16_t _x = LAYER_UPDATE_X1; _x < LAYER_UPDATE_X2; ++_x) {
					const int _pos = (_y * CANVAS_W + _x);
					const COLOR brush_color = BRUSH_PIXELS[_pos];
					const COLOR dest_color = layer_data[_pos];

					const COLOR empty{0, 0, 0, 0};

					BRUSH_PIXELS[_pos] = empty; // clear the brush pixel

					if (brush_color == empty) // if there's an empty pixel in the brush texture
					{
						_u.set(_x - LAYER_UPDATE_X1, _y - LAYER_UPDATE_Y1, dest_color, dest_color);
						continue;
					}

					if (CURRENT_TOOL == 1) // if it's the erase tool
					{
						layer_data[_pos] = empty; // erase the destination pixel
						_u.set(_x - LAYER_UPDATE_X1, _y - LAYER_UPDATE_Y1, dest_color, empty);
						continue;
					}

					if (dest_color == empty) // if destination pixel is empty
					{
						layer_data[_pos] = brush_color; // make destination the saved brush pixel
						_u.set(_x - LAYER_UPDATE_X1, _y - LAYER_UPDATE_Y1, dest_color, brush_color);
						continue;
					}

					// if it isn't any of those edge cases, we properly mix the colours
					const COLOR new_col = blend_colors(brush_color, dest_color);
					layer_data[_pos] = new_col;
					_u.set(_x - LAYER_UPDATE_X1, _y - LAYER_UPDATE_Y1, dest_color, new_col);
				}
			}

			// clear the brush texture (since we made all pixels 0x00000000)
			const SDL_Rect dirty_rect { LAYER_UPDATE_X1, LAYER_UPDATE_Y1, t_layer_update_w, t_layer_update_h };

			SDL_SetTextureBlendMode(BRUSH_TEXTURE, SDL_BLENDMODE_NONE);
			SDL_UpdateTexture(BRUSH_TEXTURE, &dirty_rect, &BRUSH_PIXELS[LAYER_UPDATE_Y1 * CANVAS_W + LAYER_UPDATE_X1], CANVAS_PITCH);

			// if we're back a few steps in the undo reel, we clear all the above undo steps.
			while (UNDO_POS > 0) {
				UNDO_LIST.pop_back();
				UNDO_POS--;
			};

			// add the new undo
			UNDO_LIST.push_back(std::move(_u));
			
			// update the layer we drew to
			SDL_UpdateTexture(LAYERS[CURRENT_LAYER].texture, &dirty_rect, &layer_data[LAYER_UPDATE_Y1 * CANVAS_W + LAYER_UPDATE_X1], CANVAS_PITCH);

			LAYER_UPDATE = 0;
		}
		else
		{
			if (LAYER_UPDATE > 0) LAYER_UPDATE--;

			if (LAYER_UPDATE == 0)
			{
				LAYER_UPDATE_X1 = INT16_MAX;
				LAYER_UPDATE_Y1 = INT16_MAX;
				LAYER_UPDATE_X2 = INT16_MIN;
				LAYER_UPDATE_Y2 = INT16_MIN;

				LAYER_UPDATE = -1;
			}
		}

		// CANVAS UPDATE
		if (CANVAS_UPDATE) {
			if (UNDO_UPDATE)
			{
				SDL_SetTextureBlendMode(LAYERS[UNDO_UPDATE_LAYER].texture, SDL_BLENDMODE_NONE);
				UNDO_UPDATE_RECT.x = (clamp(UNDO_UPDATE_RECT.x, 0, CANVAS_W - 1));
				UNDO_UPDATE_RECT.y = (clamp(UNDO_UPDATE_RECT.y, 0, CANVAS_H - 1));
				UNDO_UPDATE_RECT.w = (clamp(UNDO_UPDATE_RECT.w, 1, CANVAS_W));
				UNDO_UPDATE_RECT.h = (clamp(UNDO_UPDATE_RECT.h, 1, CANVAS_H));
				SDL_UpdateTexture(LAYERS[UNDO_UPDATE_LAYER].texture, &UNDO_UPDATE_RECT, &LAYERS[UNDO_UPDATE_LAYER].pixels[UNDO_UPDATE_RECT.y * CANVAS_W + UNDO_UPDATE_RECT.x], CANVAS_PITCH);
			}
			else
			{
				for (const auto& layer: LAYERS) {
					SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_NONE);
					SDL_UpdateTexture(layer.texture, nullptr, &layer.pixels[0], CANVAS_PITCH);
				}
			}

			SDL_UpdateTexture(BRUSH_TEXTURE, nullptr, BRUSH_PIXELS.get(), CANVAS_PITCH);
			CANVAS_UPDATE = 0;
			UNDO_UPDATE = 0;
			UNDO_UPDATE_LAYER = 0;
		}


		SDL_SetRenderDrawColor(RENDERER, 255, 255, 255, 255);

		// RENDER
		// smooth lerping animation to make things SLIGHTLY smooth when panning and zooming
		// the '4.0' can be any number, and will be a changeable option in Settings
		float anim_speed = 4.0f;
		CANVAS_X_ANIM = (reach_tween(CANVAS_X_ANIM, floor(CANVAS_X), anim_speed));
		CANVAS_Y_ANIM = (reach_tween(CANVAS_Y_ANIM, floor(CANVAS_Y), anim_speed));
		CANVAS_W_ANIM = (reach_tween(CANVAS_W_ANIM, floor((float)CANVAS_W * CANVAS_ZOOM), anim_speed));
		CANVAS_H_ANIM = (reach_tween(CANVAS_H_ANIM, floor((float)CANVAS_H * CANVAS_ZOOM), anim_speed));
		CELL_W_ANIM = (reach_tween(CELL_W_ANIM, floor((float)CELL_W * CANVAS_ZOOM), anim_speed));
		CELL_H_ANIM = (reach_tween(CELL_H_ANIM, floor((float)CELL_H * CANVAS_ZOOM), anim_speed));
		
		SDL_FRect F_RECT {};

		// transparent background grid
		float bg_w = (ceil(CANVAS_W_ANIM / CELL_W_ANIM) * CELL_W_ANIM);
		float bg_h = (ceil(CANVAS_H_ANIM / CELL_H_ANIM) * CELL_H_ANIM);
		F_RECT = SDL_FRect {CANVAS_X_ANIM, CANVAS_Y_ANIM, bg_w, bg_h};
		SDL_SetTextureBlendMode(BG_GRID_TEXTURE, SDL_BLENDMODE_NONE);
		SDL_RenderCopyF(RENDERER, BG_GRID_TEXTURE, nullptr, &F_RECT);

		// these 2 rects cover the overhang the background grid has beyond the canvas
		SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 255);
		F_RECT = SDL_FRect {
			std::max(0.0f, CANVAS_X_ANIM), std::max(0.0f,CANVAS_Y_ANIM + (CANVAS_H_ANIM)),
			std::min(float(WINDOW_W),bg_w), CELL_H * CANVAS_ZOOM
		};
		SDL_RenderFillRectF(RENDERER, &F_RECT);
		F_RECT = SDL_FRect {
			std::max(0.0f, CANVAS_X_ANIM + (CANVAS_W_ANIM)), std::max(0.0f, CANVAS_Y_ANIM),
			CELL_W * CANVAS_ZOOM, std::min(float(WINDOW_H),bg_h)
		};
		SDL_RenderFillRectF(RENDERER, &F_RECT);
		
		F_RECT = {CANVAS_X_ANIM, CANVAS_Y_ANIM, CANVAS_W_ANIM, CANVAS_H_ANIM};

		// RENDER THE LAYERS
		for (uint16_t i = 0; i < LAYERS.size(); i++)
		{
			const LAYER_INFO& layer = LAYERS[i];
			SDL_SetTextureBlendMode(layer.texture, layer.blendmode);
			SDL_RenderCopyF(RENDERER, layer.texture, nullptr, &F_RECT);
			if (i == CURRENT_LAYER)
			{
				if (BRUSH_UPDATE)
				{
					SDL_SetTextureBlendMode(BRUSH_TEXTURE, SDL_BLENDMODE_BLEND);
					SDL_RenderCopyF(RENDERER, BRUSH_TEXTURE, nullptr, &F_RECT);
				}
			}
		}

		// the grey box around the canvas
		SDL_SetRenderDrawColor(RENDERER, 64, 64, 64, 255);
		F_RECT = { CANVAS_X_ANIM - 2.0f, CANVAS_Y_ANIM - 2.0f, CANVAS_W_ANIM + 4.0f, CANVAS_H_ANIM + 4.0f };
		SDL_RenderDrawRectF(RENDERER, &F_RECT);

		// RENDER THE UI BOXES
		int16_t t_UIBOX_IN = UIBOX_IN;
		UIBOX_IN = -1;
		int16_t t_ELEMENT_IN = ELEMENT_IN;
		ELEMENT_IN = -1;
		//int16_t _uiborder_over = -1;
		const int16_t _uiboxes_size = UIBOXES.size() - 1;
		//if (!MOUSEBUTTON_LEFT) UIBOX_CLICKED_IN = -1;
		SDL_Rect RECT{};
		int16_t _uibox_id = 0;
		for (int16_t i = 0; i <= _uiboxes_size; i++)
		{
			_uibox_id = _uiboxes_size - i;
			UIBOX_INFO* uibox = &UIBOXES[_uibox_id];

			uibox->x = clamp(uibox->x, 0, WINDOW_W - uibox->w);
			uibox->y = clamp(uibox->y, 0, WINDOW_H - uibox->h);

			RECT = { uibox->x, uibox->y, uibox->w, uibox->h };
			//SDL_SetRenderDrawColor(RENDERER, 16, 16, 16, 255);
			//SDL_RenderFillRect(RENDERER, &RECT);
			//SDL_SetRenderDrawColor(RENDERER, 64, 64, 64, 255);
			//SDL_RenderDrawRect(RENDERER, &RECT);

			//uibox->chars[18] = CHAR_BOXH;
			SDL_Rect rect{};
			UIBOX_CHARINFO* _charinfo;

			int _uibox_w = uibox->chr_w;
			int _uibox_h = uibox->chr_h;

			static int _mpx, _mpy;

			// MOUSE IS IN WINDOW
			if (uibox->element_update || ((UIBOX_CLICKED_IN == -1 || UIBOX_CLICKED_IN == t_UIBOX_IN) && (point_in_rect(MOUSE_X, MOUSE_Y, uibox->x, uibox->y, uibox->w, uibox->h)
				|| point_in_rect(MOUSE_PREVX, MOUSE_PREVY, uibox->x, uibox->y, uibox->w, uibox->h))))
			{
				UIBOX_IN = _uibox_id;

				// GRABBABLE TOP BAR
				if (uibox->grabbable) {
					if (point_in_rect(MOUSE_X, MOUSE_Y, uibox->x, uibox->y, uibox->w - (FONT_CHRW * 4), FONT_CHRH))
					{
						if (!uibox->in_topbar)
						{
							for (uint16_t j = 0; j < _uibox_w - 4; j++)
							{
								_charinfo = &uibox->charinfo[j];
								_charinfo->col = COLOR{ 0,0,0,255 };
								_charinfo->bg_col = COLOR{ 255,0,64,255 };
								uibox->update_stack.insert(uibox->update_stack.begin() + (rand() % (uibox->update_stack.size() + 1)), j);
							}
							uibox->in_topbar = true;
							uibox->update = true;
						}
					}
					else
					{
						if (uibox->in_topbar)
						{
							for (int j = 0; j < _uibox_w - 4; j++)
							{
								_charinfo = &uibox->charinfo[j];
								_charinfo->col = COL_WHITE;
								_charinfo->bg_col = COLOR{ 0,0,0,1 };
								uibox->update_stack.insert(uibox->update_stack.begin() + (rand() % (uibox->update_stack.size() + 1)), j);
							}
							uibox->in_topbar = false;
							uibox->update = true;
						}
					}
				}

				// SHRINK BUTTON
				if (point_in_rect(MOUSE_X, MOUSE_Y, uibox->x + (uibox->w - (FONT_CHRW * 4)), uibox->y, FONT_CHRW * 3, FONT_CHRH))
				{
					if (!uibox->in_shrink)
					{
						for (uint16_t j = _uibox_w - 4; j < _uibox_w - 1; j++)
						{
							_charinfo = &uibox->charinfo[j];
							_charinfo->col = COLOR{ 0,0,0,255 };
							_charinfo->bg_col = COLOR{ 255,0,64,255 };
							uibox->update_stack.insert(uibox->update_stack.begin() + (rand() % (uibox->update_stack.size() + 1)), j);
						}
						uibox->in_shrink = true;
						uibox->update = true;
					}
				}
				else
				{
					if (uibox->in_shrink)
					{
						for (int j = _uibox_w - 4; j < _uibox_w - 1; j++)
						{
							_charinfo = &uibox->charinfo[j];
							_charinfo->col = COL_WHITE;
							_charinfo->bg_col = COLOR{ 0,0,0,1 };
							uibox->update_stack.insert(uibox->update_stack.begin() + (rand() % (uibox->update_stack.size() + 1)), j);
						}
						uibox->in_shrink = false;
						uibox->update = true;
					}
				}

				if ((uibox->element_update || UIBOX_IN == UIBOX_PREVIN) && !uibox->shrink)
				{
					UIBOX_ELEMENT* _element;
					for (int e = 0; e < uibox->element.size(); e++)
					{
						_element = &uibox->element[e];
						if ((uibox->element_update && ((_element->input_int == nullptr && (bool)(*_element->input_bool)) || (_element->input_int != nullptr && *_element->input_int == _element->input_int_var))) ||
							((!_element->is_pos && point_in_rect(MOUSE_X, MOUSE_Y, uibox->x + (FONT_CHRW * 2), uibox->y + ((e + 2) * FONT_CHRH), uibox->w - (FONT_CHRW * 4), FONT_CHRH)) ||
							_element->is_pos && point_in_rect(MOUSE_X, MOUSE_Y, uibox->x + (FONT_CHRW * _element->px), uibox->y + (FONT_CHRH * _element->py), FONT_CHRW * _element->text.size(), FONT_CHRH)))
						{
							if(!uibox->element_update) ELEMENT_IN = e;
							if (_element->over && !uibox->element_update) continue; // if mouse is already over, don't update

							for (uint16_t ej = 0; ej < _uibox_w; ej++)
							{
								if (((!_element->is_pos) && ej >= (_uibox_w - 4)) || ((_element->is_pos) && ej >= _element->text.size())) break;
								uint16_t _tj = (!_element->is_pos)?(ej + 2 + ((e + 2) * _uibox_w)):(_element->px + (_element->py * _uibox_w) + ej);
								_charinfo = &uibox->charinfo[_tj];
								_charinfo->col = COL_BLACK;// (!_element->is_pos) ? COL_BLACK : (!(bool)(*_element->input_bool) ? COL_BLACK : COLOR{ 255,0,64,255 });
								_charinfo->bg_col = COLOR{ 255,0,64,255 };
								if ((_element->input_int == nullptr && (bool)(*_element->input_bool)) || (_element->input_int != nullptr && *_element->input_int == _element->input_int_var))
								{
									if (ej < _element->over_text.size()) _charinfo->chr = (_element->over_text.c_str())[ej]; else _charinfo->chr = 32;
								}
								uibox->update_stack.insert(uibox->update_stack.begin() + (rand() % (uibox->update_stack.size() + 1)), _tj);
							}
							
							uibox->update = true;
							_element->over = true;
						}
						else
						if (_element->over || uibox->element_update)
						{
							if (((_element->input_int == nullptr && !(bool)(*_element->input_bool)) || (_element->input_int != nullptr && *_element->input_int != _element->input_int_var)) || uibox->element_update)
							{
								for (uint16_t ej = 0; ej < _uibox_w; ej++)
								{
									if (((!_element->is_pos) && ej >= (_uibox_w - 4)) || ((_element->is_pos) && ej >= _element->text.size())) break;
									uint16_t _tj = (!_element->is_pos) ? (ej + 2 + ((e + 2) * _uibox_w)) : (_element->px + (_element->py * _uibox_w) + ej);
									_charinfo = &uibox->charinfo[_tj];
									_charinfo->col = COL_WHITE;
									_charinfo->bg_col = COLOR{ 0,0,0,1 };
									if ((_element->input_int == nullptr && !(bool)(*_element->input_bool)) || (_element->input_int != nullptr && *_element->input_int == _element->input_int_var))
									{
										if (ej < _element->text.size()) _charinfo->chr = (_element->text.c_str())[ej]; else _charinfo->chr = 32;
									}
									uibox->update_stack.insert(uibox->update_stack.begin() + (rand() % (uibox->update_stack.size() + 1)), _tj);
								}
								uibox->update = true;
							}
							_element->over = false;
						}
					}
					uibox->element_update = false;
				}
			}

			if (uibox->update)
			{
				if (uibox->texture == nullptr)
				{
					uibox->texture = SDL_CreateTexture(RENDERER, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, uibox->w, uibox->h);
				}

				SDL_SetTextureBlendMode(uibox->texture, SDL_BLENDMODE_BLEND);
				SDL_SetTextureBlendMode(FONTMAP, SDL_BLENDMODE_BLEND);
				SDL_SetRenderTarget(RENDERER, uibox->texture);
				SDL_SetRenderDrawColor(RENDERER, 255, 255, 255, 255);
				
				SDL_Rect chr_rect{};

				uint16_t j;
				do { // do the whole loop so we don't have to wait for the windows to appear upon opening the app
					if (uibox->update_stack.size())
					{
						if (uibox->update_tick <= 0)
						{
							do
							{
								j = uibox->update_stack.front();
								uibox->update_stack.pop_front();
								_charinfo = &uibox->charinfo[j];
							} while (uibox->update_stack.size()>1 && _charinfo->chr == 32 && !_charinfo->bg_col.a); // skip Space chars, but not if it has to draw a bg

							rect = { (j % _uibox_w) * FONT_CHRW, (j / _uibox_w) * FONT_CHRH, FONT_CHRW, FONT_CHRH };

							if (_charinfo->bg_col.a) // if the bg_col isn't 0
							{
								chr_rect = { 0xdb * FONT_CHRW, 0, FONT_CHRW, FONT_CHRH }; // the SOLID_BLOCK char is the bg block
								SDL_SetTextureColorMod(FONTMAP, _charinfo->bg_col.r, _charinfo->bg_col.g, _charinfo->bg_col.b);
								SDL_RenderCopy(RENDERER, FONTMAP, &chr_rect, &rect);
								if (_charinfo->bg_col.a == 1) _charinfo->bg_col.a = 0; // if it's a quick "return to black", only do it once
							}

							chr_rect = { _charinfo->chr * FONT_CHRW, 0, FONT_CHRW, FONT_CHRH };
							SDL_SetTextureColorMod(FONTMAP, _charinfo->col.r, _charinfo->col.g, _charinfo->col.b);
							SDL_RenderCopy(RENDERER, FONTMAP, &chr_rect, &rect);
							uibox->update_tick = 4; // Can be any number. Bigger number = slower manifest animation
						}
						else uibox->update_tick--;
					}
					else
					{
						uibox->update = false;
						uibox->update_creation = false;
					}
				} while (uibox->update_creation); // only happens the moment a window is created

				SDL_SetRenderTarget(RENDERER, NULL);
			}

			if (uibox->shrink)
			{
				SDL_Rect shrink_rect = {0,0,uibox->w,FONT_CHRH};
				rect = { uibox->x, uibox->y, uibox->w, FONT_CHRH };
				SDL_RenderCopy(RENDERER, uibox->texture, &shrink_rect, &rect);
			}
			else
			{
				rect = { uibox->x, uibox->y, uibox->w, uibox->h };
				SDL_RenderCopy(RENDERER, uibox->texture, NULL, &rect);
			}
		}

		UIBOX_PREVIN = UIBOX_IN;

		SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 255);

		// TEST HUE BAR
		//const SDL_Rect temp_rect{10,10,16,360};
		//SDL_RenderCopy(RENDERER, UI_TEXTURE_HUEBAR, nullptr, &temp_rect);

		if (BRUSH_LIST[BRUSH_LIST_POS]->alpha[0]) FC_Draw(font, RENDERER, 200, 10, "ON");
		//FC_Draw(font, RENDERER, 36, 30, "%i\n%i", UIBOX_IN, UIBOX_CLICKED_IN);
		
		SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 0);
		SDL_RenderPresent(RENDERER);
        
#if __APPLE__
        SDL_Delay(1);
#endif
		//

		// SET FPS
		static int fps_rate = 0;
        const Uint64 fps_end = SDL_GetPerformanceCounter();
        const static Uint64 fps_freq = SDL_GetPerformanceFrequency();
        const double fps_seconds = (fps_end - fps_start) / static_cast<double>(fps_freq);
        FPS = reach_tween(FPS, 1 / (float)fps_seconds, 100.0);
        if (fps_rate <= 0) {
            std::cout << " FPS: " << (int)floor(FPS) << "       " << '\r';
            fps_rate = 60 * 4;
        } else fps_rate--;
	}

	SDL_Delay(10);

	TTF_CloseFont(FONT);
	FC_FreeFont(font);
	FC_FreeFont(font_bold);
	SDL_DestroyRenderer(RENDERER);
	SDL_DestroyWindow(WINDOW);
	SDL_Quit();
	TTF_Quit();

	return 0;
}

  //
 //   END   ///////////////////////////////////////////////// ///////  //////   /////    ////     ///      //       /
//
