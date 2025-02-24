/*************************************************************************/
/*  plugin_config_dialog.cpp                                             */
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

#include "plugin_config_dialog.h"

#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "editor/editor_scale.h"
#include "editor/project_settings_editor.h"

void PluginConfigDialog::_clear_fields() {
	name_edit->set_text("");
	subfolder_edit->set_text("");
	desc_edit->set_text("");
	author_edit->set_text("");
	version_edit->set_text("");
	script_edit->set_text("");
}

void PluginConfigDialog::_on_confirmed() {
	String path = "res://addons/" + _get_subfolder();

	if (!_edit_mode) {
		Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		if (d.is_null() || d->make_dir_recursive(path) != OK) {
			return;
		}
	}

	int lang_idx = script_option_edit->get_selected();
	String ext = ScriptServer::get_language(lang_idx)->get_extension();
	String script_name = script_edit->get_text().is_empty() ? _get_subfolder() : script_edit->get_text();
	if (script_name.get_extension().is_empty()) {
		script_name += "." + ext;
	}
	String script_path = path.plus_file(script_name);

	Ref<ConfigFile> cf = memnew(ConfigFile);
	cf->set_value("plugin", "name", name_edit->get_text());
	cf->set_value("plugin", "description", desc_edit->get_text());
	cf->set_value("plugin", "author", author_edit->get_text());
	cf->set_value("plugin", "version", version_edit->get_text());
	cf->set_value("plugin", "script", script_name);

	cf->save(path.plus_file("plugin.cfg"));

	if (!_edit_mode) {
		String class_name = script_name.get_basename();
		String template_content = "";
		Vector<ScriptLanguage::ScriptTemplate> templates = ScriptServer::get_language(lang_idx)->get_built_in_templates("EditorPlugin");
		if (!templates.is_empty()) {
			template_content = templates[0].content;
		}
		Ref<Script> script = ScriptServer::get_language(lang_idx)->make_template(template_content, class_name, "EditorPlugin");
		script->set_path(script_path);
		ResourceSaver::save(script_path, script);

		emit_signal(SNAME("plugin_ready"), script.ptr(), active_edit->is_pressed() ? _to_absolute_plugin_path(_get_subfolder()) : "");
	} else {
		EditorNode::get_singleton()->get_project_settings()->update_plugins();
	}
	_clear_fields();
}

void PluginConfigDialog::_on_cancelled() {
	_clear_fields();
}

void PluginConfigDialog::_on_language_changed(const int) {
	_on_required_text_changed(String());
}

void PluginConfigDialog::_on_required_text_changed(const String &) {
	int lang_idx = script_option_edit->get_selected();
	String ext = ScriptServer::get_language(lang_idx)->get_extension();

	Ref<Texture2D> valid_icon = get_theme_icon(SNAME("StatusSuccess"), SNAME("EditorIcons"));
	Ref<Texture2D> invalid_icon = get_theme_icon(SNAME("StatusWarning"), SNAME("EditorIcons"));

	// Set variables to assume all is valid
	bool is_valid = true;
	name_validation->set_texture(valid_icon);
	subfolder_validation->set_texture(valid_icon);
	script_validation->set_texture(valid_icon);
	name_validation->set_tooltip("");
	subfolder_validation->set_tooltip("");
	script_validation->set_tooltip("");

	// Change valid status to invalid depending on conditions.
	Vector<String> errors;
	if (name_edit->get_text().is_empty()) {
		is_valid = false;
		name_validation->set_texture(invalid_icon);
		name_validation->set_tooltip(TTR("Plugin name cannot be blank."));
	}
	if ((!script_edit->get_text().get_extension().is_empty() && script_edit->get_text().get_extension() != ext) || script_edit->get_text().ends_with(".")) {
		is_valid = false;
		script_validation->set_texture(invalid_icon);
		script_validation->set_tooltip(vformat(TTR("Script extension must match chosen language extension (.%s)."), ext));
	}
	if (!subfolder_edit->get_text().is_empty() && !subfolder_edit->get_text().is_valid_filename()) {
		is_valid = false;
		subfolder_validation->set_texture(invalid_icon);
		subfolder_validation->set_tooltip(TTR("Subfolder name is not a valid folder name."));
	} else {
		String path = "res://addons/" + _get_subfolder();
		if (!_edit_mode && DirAccess::exists(path)) { // Only show this error if in "create" mode.
			is_valid = false;
			subfolder_validation->set_texture(invalid_icon);
			subfolder_validation->set_tooltip(TTR("Subfolder cannot be one which already exists."));
		}
	}

	get_ok_button()->set_disabled(!is_valid);
}

String PluginConfigDialog::_get_subfolder() {
	return subfolder_edit->get_text().is_empty() ? name_edit->get_text().replace(" ", "_").to_lower() : subfolder_edit->get_text();
}

String PluginConfigDialog::_to_absolute_plugin_path(const String &p_plugin_name) {
	return "res://addons/" + p_plugin_name + "/plugin.cfg";
}

void PluginConfigDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				name_edit->grab_focus();
			}
		} break;

		case NOTIFICATION_READY: {
			connect("confirmed", callable_mp(this, &PluginConfigDialog::_on_confirmed));
			get_cancel_button()->connect("pressed", callable_mp(this, &PluginConfigDialog::_on_cancelled));
		} break;
	}
}

void PluginConfigDialog::config(const String &p_config_path) {
	if (p_config_path.length()) {
		Ref<ConfigFile> cf = memnew(ConfigFile);
		Error err = cf->load(p_config_path);
		ERR_FAIL_COND_MSG(err != OK, "Cannot load config file from path '" + p_config_path + "'.");

		name_edit->set_text(cf->get_value("plugin", "name", ""));
		subfolder_edit->set_text(p_config_path.get_base_dir().get_basename().get_file());
		desc_edit->set_text(cf->get_value("plugin", "description", ""));
		author_edit->set_text(cf->get_value("plugin", "author", ""));
		version_edit->set_text(cf->get_value("plugin", "version", ""));
		script_edit->set_text(cf->get_value("plugin", "script", ""));

		_edit_mode = true;
		active_edit->hide();
		Object::cast_to<Label>(active_edit->get_parent()->get_child(active_edit->get_index() - 2))->hide();
		subfolder_edit->hide();
		subfolder_validation->hide();
		Object::cast_to<Label>(subfolder_edit->get_parent()->get_child(subfolder_edit->get_index() - 2))->hide();
		set_title(TTR("Edit a Plugin"));
	} else {
		_clear_fields();
		_edit_mode = false;
		active_edit->show();
		Object::cast_to<Label>(active_edit->get_parent()->get_child(active_edit->get_index() - 2))->show();
		subfolder_edit->show();
		subfolder_validation->show();
		Object::cast_to<Label>(subfolder_edit->get_parent()->get_child(subfolder_edit->get_index() - 2))->show();
		set_title(TTR("Create a Plugin"));
	}
	// Simulate text changing so the errors populate.
	_on_required_text_changed("");

	get_ok_button()->set_disabled(!_edit_mode);
	get_ok_button()->set_text(_edit_mode ? TTR("Update") : TTR("Create"));
}

void PluginConfigDialog::_bind_methods() {
	ADD_SIGNAL(MethodInfo("plugin_ready", PropertyInfo(Variant::STRING, "script_path", PROPERTY_HINT_NONE, ""), PropertyInfo(Variant::STRING, "activate_name")));
}

PluginConfigDialog::PluginConfigDialog() {
	get_ok_button()->set_disabled(true);
	set_hide_on_ok(true);

	VBoxContainer *vbox = memnew(VBoxContainer);
	vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(vbox);

	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(3);
	vbox->add_child(grid);

	// Plugin Name
	Label *name_lb = memnew(Label);
	name_lb->set_text(TTR("Plugin Name:"));
	name_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(name_lb);

	name_validation = memnew(TextureRect);
	name_validation->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	grid->add_child(name_validation);

	name_edit = memnew(LineEdit);
	name_edit->connect("text_changed", callable_mp(this, &PluginConfigDialog::_on_required_text_changed));
	name_edit->set_placeholder("MyPlugin");
	grid->add_child(name_edit);

	// Subfolder
	Label *subfolder_lb = memnew(Label);
	subfolder_lb->set_text(TTR("Subfolder:"));
	subfolder_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(subfolder_lb);

	subfolder_validation = memnew(TextureRect);
	subfolder_validation->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	grid->add_child(subfolder_validation);

	subfolder_edit = memnew(LineEdit);
	subfolder_edit->set_placeholder("\"my_plugin\" -> res://addons/my_plugin");
	subfolder_edit->connect("text_changed", callable_mp(this, &PluginConfigDialog::_on_required_text_changed));
	grid->add_child(subfolder_edit);

	// Description
	Label *desc_lb = memnew(Label);
	desc_lb->set_text(TTR("Description:"));
	desc_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(desc_lb);

	Control *desc_spacer = memnew(Control);
	grid->add_child(desc_spacer);

	desc_edit = memnew(TextEdit);
	desc_edit->set_custom_minimum_size(Size2(400, 80) * EDSCALE);
	desc_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
	grid->add_child(desc_edit);

	// Author
	Label *author_lb = memnew(Label);
	author_lb->set_text(TTR("Author:"));
	author_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(author_lb);

	Control *author_spacer = memnew(Control);
	grid->add_child(author_spacer);

	author_edit = memnew(LineEdit);
	author_edit->set_placeholder("Godette");
	grid->add_child(author_edit);

	// Version
	Label *version_lb = memnew(Label);
	version_lb->set_text(TTR("Version:"));
	version_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(version_lb);

	Control *version_spacer = memnew(Control);
	grid->add_child(version_spacer);

	version_edit = memnew(LineEdit);
	version_edit->set_placeholder("1.0");
	grid->add_child(version_edit);

	// Language dropdown
	Label *script_option_lb = memnew(Label);
	script_option_lb->set_text(TTR("Language:"));
	script_option_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(script_option_lb);

	Control *script_opt_spacer = memnew(Control);
	grid->add_child(script_opt_spacer);

	script_option_edit = memnew(OptionButton);
	int default_lang = 0;
	for (int i = 0; i < ScriptServer::get_language_count(); i++) {
		ScriptLanguage *lang = ScriptServer::get_language(i);
		script_option_edit->add_item(lang->get_name());
		if (lang->get_name() == "GDScript") {
			default_lang = i;
		}
	}
	script_option_edit->select(default_lang);
	grid->add_child(script_option_edit);
	script_option_edit->connect("item_selected", callable_mp(this, &PluginConfigDialog::_on_language_changed));

	// Plugin Script Name
	Label *script_lb = memnew(Label);
	script_lb->set_text(TTR("Script Name:"));
	script_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(script_lb);

	script_validation = memnew(TextureRect);
	script_validation->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	grid->add_child(script_validation);

	script_edit = memnew(LineEdit);
	script_edit->connect("text_changed", callable_mp(this, &PluginConfigDialog::_on_required_text_changed));
	script_edit->set_placeholder("\"plugin.gd\" -> res://addons/my_plugin/plugin.gd");
	grid->add_child(script_edit);

	// Activate now checkbox
	// TODO Make this option work better with languages like C#. Right now, it does not work because the C# project must be compiled first.
	Label *active_lb = memnew(Label);
	active_lb->set_text(TTR("Activate now?"));
	active_lb->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	grid->add_child(active_lb);

	Control *active_spacer = memnew(Control);
	grid->add_child(active_spacer);

	active_edit = memnew(CheckBox);
	active_edit->set_pressed(true);
	grid->add_child(active_edit);
}

PluginConfigDialog::~PluginConfigDialog() {
}
