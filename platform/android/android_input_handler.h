/*************************************************************************/
/*  android_input_handler.h                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef ANDROID_INPUT_HANDLER_H
#define ANDROID_INPUT_HANDLER_H

#include "core/input/input.h"

// This class encapsulates all the handling of input events that come from the Android UI thread.
// Remarks:
// - It's not thread-safe by itself, so its functions must only be called on a single thread, which is the Android UI thread.
// - Its functions must only call thread-safe methods.
class AndroidInputHandler {
public:
	struct TouchPos {
		int id = 0;
		Point2 pos;
	};

	enum {
		JOY_EVENT_BUTTON = 0,
		JOY_EVENT_AXIS = 1,
		JOY_EVENT_HAT = 2
	};

	struct JoypadEvent {
		int device = 0;
		int type = 0;
		int index = 0; // Can be either JoyAxis or JoyButton.
		bool pressed = false;
		float value = 0;
		HatMask hat = HatMask::CENTER;
	};

private:
	bool alt_mem = false;
	bool shift_mem = false;
	bool control_mem = false;
	bool meta_mem = false;

	MouseButton buttons_state = MouseButton::NONE;

	Vector<TouchPos> touch;
	Point2 hover_prev_pos; // needed to calculate the relative position on hover events
	Point2 scroll_prev_pos; // needed to calculate the relative position on scroll events

	void _set_key_modifier_state(Ref<InputEventWithModifiers> ev);

	static MouseButton _button_index_from_mask(MouseButton button_mask);
	static MouseButton _android_button_mask_to_godot_button_mask(int android_button_mask);

	void _wheel_button_click(MouseButton event_buttons_mask, const Ref<InputEventMouseButton> &ev, MouseButton wheel_button, float factor);

public:
	void process_touch(int p_event, int p_pointer, const Vector<TouchPos> &p_points);
	void process_hover(int p_type, Point2 p_pos);
	void process_mouse_event(int input_device, int event_action, int event_android_buttons_mask, Point2 event_pos, float event_vertical_factor = 0, float event_horizontal_factor = 0);
	void process_double_tap(int event_android_button_mask, Point2 p_pos);
	void process_scroll(Point2 p_pos);
	void process_joy_event(JoypadEvent p_event);
	void process_key_event(int p_keycode, int p_scancode, int p_unicode_char, bool p_pressed);
};

#endif // ANDROID_INPUT_HANDLER_H
