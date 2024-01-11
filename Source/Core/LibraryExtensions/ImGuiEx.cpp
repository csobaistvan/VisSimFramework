#include "PCH.h"
#include "ImGuiEx.h"

#include "StdEx.h"
#include "Core/Preprocessor.h"
#include "Core/Debug.h"

namespace ImGui
{
	////////////////////////////////////////////////////////////////////////////////
	float GetFrameWidth()
	{
		ImGuiContext& g = *GImGui;
		return g.FontSize + g.Style.FramePadding.x * 2.0f;
	}

	////////////////////////////////////////////////////////////////////////////////
	float GetFrameWidthWithSpacing()
	{
		ImGuiContext& g = *GImGui;
		return g.FontSize + g.Style.FramePadding.x * 2.0f + g.Style.ItemSpacing.x;
	}

	////////////////////////////////////////////////////////////////////////////////
	void SameLineWithinComposite()
	{
		SameLine(0, GetStyle().ItemInnerSpacing.x);
	}

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorConvertRGBtoHSV(ImVec4 rgb)
	{
		float headerColorRGB[3] = { rgb.x, rgb.y, rgb.z };
		float headerColorHSV[3] = { rgb.x, rgb.y, rgb.z };

		ColorConvertRGBtoHSV(headerColorRGB[0], headerColorRGB[1], headerColorRGB[2], headerColorHSV[0], headerColorHSV[1], headerColorHSV[2]);

		return ImVec4(headerColorHSV[0], headerColorHSV[1], headerColorHSV[2], rgb.w);
	}

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorConvertHSVtoRGB(ImVec4 hsv)
	{
		float headerColorRGB[3] = { hsv.x, hsv.y, hsv.z };
		float headerColorHSV[3] = { hsv.x, hsv.y, hsv.z };

		ColorConvertHSVtoRGB(headerColorHSV[0], headerColorHSV[1], headerColorHSV[2], headerColorRGB[0], headerColorRGB[1], headerColorRGB[2]);

		return ImVec4(headerColorRGB[0], headerColorRGB[1], headerColorRGB[2], hsv.w);
	}

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorFromGlmVector(glm::vec3 v)
	{
		return ImVec4(v.x, v.y, v.z, 1.0f);
	}

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorFromGlmVector(glm::vec4 v)
	{
		return ImVec4(v.x, v.y, v.z, v.w);
	}

	////////////////////////////////////////////////////////////////////////////////
	ImU32 Color32FromGlmVector(glm::vec3 v)
	{
		return IM_COL32(int(v.x * 255), int(v.y * 255), int(v.z * 255), 255);
	}

	////////////////////////////////////////////////////////////////////////////////
	ImU32 Color32FromGlmVector(glm::vec4 v)
	{
		return IM_COL32(int(v.x * 255), int(v.y * 255), int(v.z * 255), int(v.w * 255));
	}

	////////////////////////////////////////////////////////////////////////////////
	ImGuiID CurrentTabBarId()
	{
		return GImGui->CurrentTabBar->ID;
	}

	////////////////////////////////////////////////////////////////////////////////
	const char* CurrentTabItemName()
	{
		return GImGui->CurrentTabBar->GetTabName(&GImGui->CurrentTabBar->Tabs[GImGui->CurrentTabBar->LastTabItemIdx]);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool BeginTabItem(const char* label, const char* active_tab_name, ImGuiTabItemFlags flags)
	{
		return BeginTabItem(label, (bool*) nullptr, strcmp(label, active_tab_name) == 0 ? flags | ImGuiTabItemFlags_SetSelected : flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool TreeNodeEx(const char* label, bool* popen, ImGuiTreeNodeFlags flags)
	{
		if (popen && *popen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
		const bool isOpen = TreeNodeEx(label, flags);
		if (popen) *popen = isOpen;
		return isOpen;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool SliderFloatN(const char* label, float* v, int components, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalarN(label, ImGuiDataType_Float, v, components, &v_min, &v_max, format, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatN(const char* label, float* v, int components, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalarN(label, ImGuiDataType_Float, v, components, v_speed, &v_min, &v_max, format, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRangeN(const char* label, int n, float* vs, float v_speed, float v_min, float v_max, const char* const* formats, ImGuiSliderFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		PushID(label);
		BeginGroup();
		PushMultiItemsWidths(n, CalcItemWidth());

		bool value_changed = false;
		for (int i = 0; i < n; ++i)
		{
			PushID(i);
			value_changed |= DragFloat("##v", &vs[i], v_speed, v_min, v_max, formats ? formats[i] : "%.3f", flags);
			PopItemWidth();
			SameLine(0, g.Style.ItemInnerSpacing.x);
			PopID();
		}

		TextEx(label, FindRenderedTextEnd(label));
		EndGroup();
		PopID();
		return value_changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange2(const char* label, float* vs, float v_speed, float v_min, float v_max, const char* const* formats, ImGuiSliderFlags flags)
	{
		return DragFloatRangeN(label, 2, vs, v_speed, v_min, v_max, formats, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange3(const char* label, float* v1, float* v2, float* v3, float v_speed, float v_min, float v_max, const char* f1, const char* f2, const char* f3, ImGuiSliderFlags flags)
	{
		float values[] = { *v1, *v2, *v3 };
		const char* formats[] =
		{
			f1 ? f1 : "%.3f",
			f2 ? f2 : "%.3f",
			f3 ? f3 : "%.3f",
		};

		bool value_changed = DragFloatRange3(label, values, v_speed, v_min, v_max, formats, flags);
		if (value_changed)
		{
			*v1 = values[0];
			*v2 = values[1];
			*v3 = values[2];
		}
		return value_changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange3(const char* label, float* vs, float v_speed, float v_min, float v_max, const char* const* formats, ImGuiSliderFlags flags)
	{
		return DragFloatRangeN(label, 3, vs, v_speed, v_min, v_max, formats, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange4(const char* label, float* v1, float* v2, float* v3, float* v4, float v_speed, float v_min, float v_max, const char* f1, const char* f2, const char* f3, const char* f4, ImGuiSliderFlags flags)
	{
		float values[] = { *v1, *v2, *v3, *v4 };
		const char* formats[] =
		{
			f1 ? f1 : "%.3f",
			f2 ? f2 : "%.3f",
			f3 ? f3 : "%.3f",
			f4 ? f4 : "%.3f",
		};

		bool value_changed = DragFloatRange3(label, values, v_speed, v_min, v_max, formats, flags);
		if (value_changed)
		{
			*v1 = values[0];
			*v2 = values[1];
			*v3 = values[2];
			*v4 = values[3];
		}
		return value_changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange4(const char* label, float* vs, float v_speed, float v_min, float v_max, const char* const* formats, ImGuiSliderFlags flags)
	{
		return DragFloatRangeN(label, 4, vs, v_speed, v_min, v_max, formats, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngleN(const char* label, float* v, int components, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		bool value_changed = false;
		BeginGroup();
		PushID(label);
		PushMultiItemsWidths(components, CalcItemWidth());
		for (int i = 0; i < components; i++)
		{
			PushID(i);
			if (i > 0)
				SameLine(0, g.Style.ItemInnerSpacing.x);
			float v_deg = (v[i]) * 360.0f / (2 * IM_PI);
			value_changed |= DragFloat("", &v_deg, v_speed, v_min, v_max, format, flags);
			v[i] = v_deg * (2 * IM_PI) / 360.0f;
			PopID();
			PopItemWidth();
		}
		PopID();

		const char* label_end = FindRenderedTextEnd(label);
		if (label != label_end)
		{
			SameLine(0, g.Style.ItemInnerSpacing.x);
			TextEx(label, label_end);
		}

		EndGroup();
		return value_changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngle(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragFloatAngleN(label, v, 1, v_speed, v_min, v_max, format, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngle2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragFloatAngleN(label, v, 2, v_speed, v_min, v_max, format, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngle3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragFloatAngleN(label, v, 3, v_speed, v_min, v_max, format, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool DragComplex(const char* label, std::complex<float>* c, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		float v[2] = { c->real(), c->imag() };
		bool result = DragFloat2(label, v, v_speed, v_min, v_max, format, flags);
		c->real(v[0]);
		c->imag(v[1]);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	void Arrow(float scale, ImGuiDir_ direction)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();

		// Size of the arrow
		ImVec2 size = ImVec2(g.FontSize * scale, g.FontSize * scale);

		// Offset to position the arrow properly
		ImVec2 arrow_pos = ImVec2((g.FontSize - size.x) * 0.5f, (g.FontSize - size.y) * 0.5f + window->DC.CurrLineTextBaseOffset);

		// Place the arrow
		const ImRect bb(window->DC.CursorPos + arrow_pos, window->DC.CursorPos + arrow_pos + size);
		ItemSize(arrow_pos + size, 0.0f);
		if (!ItemAdd(bb, 0))
			return;

		// Render the arrow
		RenderArrow(window->DrawList, bb.Min, GetColorU32(ImGuiCol_Text), direction, scale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void FancyText(std::string const& text)
	{
		// TODO: implement
	}

	////////////////////////////////////////////////////////////////////////////////
	// [0..1] -> [0..1]
	static float rescale(float t, float min, float max, PlotConfig::Scale::Type type)
	{
		switch (type) {
		case PlotConfig::Scale::Linear:
			return t;
		case PlotConfig::Scale::Log10:
			return log10(ImLerp(min, max, t) / min) / log10(max / min);
		}
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// [0..1] -> [0..1]
	static float rescale_inv(float t, float min, float max, PlotConfig::Scale::Type type)
	{
		switch (type) {
		case PlotConfig::Scale::Linear:
			return t;
		case PlotConfig::Scale::Log10:
			return (pow(max / min, t) * min - min) / (max - min);
		}
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	static int cursor_to_idx(const ImVec2& pos, const ImRect& bb, const PlotConfig& conf, float x_min, float x_max)
	{
		const float t = ImClamp((pos.x - bb.Min.x) / (bb.Max.x - bb.Min.x), 0.0f, 0.9999f);
		const int v_idx = (int)(rescale_inv(t, x_min, x_max, conf.scale.type) * (conf.values.count - 1) + 0.5);
		IM_ASSERT(v_idx >= 0 && v_idx < conf.values.count);

		return v_idx;
	}

	////////////////////////////////////////////////////////////////////////////////
	// FIXME: indexing is shown values is wrong; there is one more x sample, and the first and second x values are the same
	PlotStatus PlotEx(const char* label, const PlotConfig& conf)
	{
		PlotStatus status = PlotStatus::nothing;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return status;

		const float* const* ys_list = conf.values.ys_list;
		int ys_count = conf.values.ys_count;
		const ImU32* colors = conf.values.colors;
		if (conf.values.ys != nullptr) { // draw only a single plot
			ys_list = &conf.values.ys;
			ys_count = 1;
			colors = &conf.values.color;
		}

		const PlotConfig::value_generator_fn* ysg_list = conf.values.ysg_list;
		if (conf.values.ysg != nullptr) { // draw only a single plot
			ysg_list = &conf.values.ysg;
			ys_count = 1;
			colors = &conf.values.color;
		}

		const PlotConfig::Values::Type* types = conf.values.types ? conf.values.types : &conf.values.type;

		const void* const* user_data_list = conf.values.user_data_list;
		int user_data_count = ys_count;
		if (conf.values.user_data != nullptr) { // single user data
			user_data_list = &conf.values.user_data;
			user_data_count = 1;
		}

		float scale_min = conf.scale.min;
		float scale_max = conf.scale.max;

		if (conf.scale.auto_min || conf.scale.auto_max) {
			if (conf.scale.auto_min) {
				scale_min = FLT_MAX;
			}
			if (conf.scale.auto_max) {
				scale_max = -FLT_MAX;
			}

			for (int i = 0; i < ys_count; ++i) {
				for (int n = 0; n < conf.values.count; n++) {
					const int v1_idx = n;
					const float v1 = ys_list ? ys_list[i][v1_idx] : ysg_list[i](user_data_list[ImMin(i, user_data_count - 1)], v1_idx, v1_idx);

					if (conf.scale.auto_min) {
						scale_min = ImMin(scale_min, v1);
					}

					if (conf.scale.auto_max) {
						scale_max = ImMax(scale_max, v1);
					}
				}
			}
		}

		if (fabs(scale_min - scale_max) < 1e-3f) {
			scale_min = scale_min * 0.9f;
			scale_max = scale_max * 1.1f;
		}

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		ImVec2 frame_size = conf.frame_size;

		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		if (frame_size.x == 0.0f)
			frame_size.x = CalcItemWidth();
		if (frame_size.y == 0.0f)
			frame_size.y = label_size.y + (style.FramePadding.y * 2);

		char buf[80];
		ImVec2 tick_label_size = ImVec2(0.0f, 0.0f);
		if (conf.grid_x.show) {
			if (conf.grid_x.tick_minor.label_size != 0.0f) {
				if (conf.grid_x.tick_minor.label_fn) {
					conf.grid_x.tick_minor.label_fn(user_data_list[0], buf, sizeof(buf), conf.values.count);
				} else {
					ImFormatString(buf, sizeof(buf), conf.grid_x.tick_minor.label_format, conf.values.count);
				}
				tick_label_size = ImMax(tick_label_size, g.Font->CalcTextSizeA(g.FontSize * conf.grid_x.tick_minor.label_size, FLT_MAX, 0.0f, buf) + g.Style.ItemInnerSpacing);
			}
			if (conf.grid_x.tick_major.label_size != 0.0f) {
				if (conf.grid_x.tick_major.label_fn) {
					conf.grid_x.tick_major.label_fn(user_data_list[0], buf, sizeof(buf), conf.values.count);
				} else {
					ImFormatString(buf, sizeof(buf), conf.grid_x.tick_major.label_format, conf.values.count);
				}
				tick_label_size = ImMax(tick_label_size, g.Font->CalcTextSizeA(g.FontSize * conf.grid_x.tick_major.label_size, FLT_MAX, 0.0f, buf) + g.Style.ItemInnerSpacing);
			}
		}
		if (conf.grid_y.show) {
			if (conf.grid_y.tick_minor.label_size != 0.0f) {
				if (conf.grid_y.tick_minor.label_fn) {
					conf.grid_y.tick_minor.label_fn(user_data_list[0], buf, sizeof(buf), scale_max);
				} else {
					ImFormatString(buf, sizeof(buf), conf.grid_y.tick_minor.label_format, scale_max);
				}
				tick_label_size = ImMax(tick_label_size, g.Font->CalcTextSizeA(g.FontSize * conf.grid_y.tick_minor.label_size, FLT_MAX, 0.0f, buf) + g.Style.ItemInnerSpacing);
			}
			if (conf.grid_y.tick_major.label_size != 0.0f) {
				if (conf.grid_y.tick_major.label_fn) {
					conf.grid_y.tick_major.label_fn(user_data_list[0], buf, sizeof(buf), scale_max);
				} else {
					ImFormatString(buf, sizeof(buf), conf.grid_y.tick_major.label_format, scale_max);
				}
				tick_label_size = ImMax(tick_label_size, g.Font->CalcTextSizeA(g.FontSize * conf.grid_y.tick_major.label_size, FLT_MAX, 0.0f, buf) + g.Style.ItemInnerSpacing);
			}
		}

		ImVec2 overlay_text_size = ImVec2(0.0f, 0.0f);
		if (conf.overlay.show) {
			overlay_text_size = CalcTextSize(conf.overlay.text);
		}

		const ImRect frame_bb(
			window->DC.CursorPos,
			window->DC.CursorPos + frame_size);
		const ImRect inner_bb(
			frame_bb.Min + style.FramePadding + ImVec2(tick_label_size.x, 0.0f) - ImVec2(0.0f, ImMin(0.0f, conf.overlay.position.y) * (g.Style.ItemInnerSpacing.y + overlay_text_size.y)),
			frame_bb.Max - style.FramePadding - ImVec2(0.0f, tick_label_size.y));
		const ImRect total_bb = frame_bb;
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, 0, &frame_bb))
			return status;
		const bool hovered = ItemHoverable(frame_bb, id);

		RenderFrame(
			frame_bb.Min,
			frame_bb.Max,
			GetColorU32(ImGuiCol_FrameBg),
			true,
			style.FrameRounding);

		if (conf.values.count > 0) {
			int res_w;
			if (conf.skip_small_lines)
				res_w = ImMin((int)frame_size.x, conf.values.count);
			else
				res_w = conf.values.count;
			res_w -= 1;
			int item_count = conf.values.count - 1;

			const float t_step = 1.0f / (float)res_w;
			const float inv_scale = (scale_min == scale_max) ?
				1.0f : (1.0f / (scale_max - scale_min));

			float x_min = conf.values.offset;
			float x_max = conf.values.offset + conf.values.count - 1;
			if (conf.values.xs) {
				x_min = conf.values.xs[size_t(x_min)];
				x_max = conf.values.xs[size_t(x_max)];
			}
					   
			// Tooltip on hover
			int v_hovered = -1;
			if (conf.tooltip.show && hovered && inner_bb.Contains(g.IO.MousePos)) {
				const int v_idx = cursor_to_idx(g.IO.MousePos, inner_bb, conf, x_min, x_max);
				const size_t data_idx = conf.values.offset + (v_idx % conf.values.count);
				const float x0 = conf.values.xs ? conf.values.xs[data_idx] : data_idx;
				const float y0 = ys_list ? ys_list[0][data_idx] : ysg_list[0](user_data_list[ImMin(0, user_data_count - 1)], v_idx, x0); // TODO: tooltip is only shown for the first y-value!
				if (conf.tooltip.generator) {
					conf.tooltip.generator(user_data_list[ImMin(0, user_data_count - 1)], v_idx, x0, y0);
				} else {
					SetTooltip(conf.tooltip.format, x0, y0);
				}
				v_hovered = v_idx;
			}
			
			// draw the grids
			if (conf.grid_x.show) {
				int y0 = inner_bb.Min.y;
				int y1 = inner_bb.Max.y;
				ImU32 col_major = conf.grid_x.tick_major.color ? conf.grid_x.tick_major.color : IM_COL32(200, 200, 200, 255);
				ImU32 col_minor = conf.grid_x.tick_minor.color ? conf.grid_x.tick_minor.color : IM_COL32(200, 200, 200, 127);
				ImU32 col_label_major = conf.grid_x.tick_major.label_color ? conf.grid_x.tick_major.label_color : IM_COL32(200, 200, 200, 255);
				ImU32 col_label_minor = conf.grid_x.tick_minor.label_color ? conf.grid_x.tick_minor.label_color : IM_COL32(200, 200, 200, 127);
				float thickness_major = conf.grid_x.tick_major.thickness;
				float thickness_minor = conf.grid_x.tick_minor.thickness;
				switch (conf.scale.type) {
				case PlotConfig::Scale::Linear: {
					int ticks = conf.grid_x.ticks;
					int subticks = conf.grid_x.subticks;
					float tick_size = conf.grid_x.size;

					if (ticks != -1) {
						tick_size = (x_max - x_min) / ((ImMax(1, ticks) * ImMax(1, subticks)));
					}
					if (conf.grid_x.sanitize_fn) {
						tick_size = conf.grid_x.sanitize_fn(user_data_list[0], conf.values.count, ticks, subticks, scale_min, scale_max);
					}
					float cnt = conf.values.count / (tick_size / subticks);
					float inc = 1.f / cnt;
					float sinc = (tick_size / subticks);
					for (int i = 0; i <= cnt; ++i) {
						int x0 = ImLerp(inner_bb.Min.x, inner_bb.Max.x, i * inc);
						float xs = conf.values.offset + i * sinc;

						bool is_sub = subticks > 1 && i % subticks;
						ImU32 color = is_sub ? col_minor : col_major;
						ImU32 label_color = is_sub ? col_label_minor : col_label_major;
						float thickness = is_sub ? thickness_minor : thickness_major;
						float label_text_size = is_sub ? conf.grid_x.tick_minor.label_size : conf.grid_x.tick_major.label_size;
						auto label_fn = is_sub ? conf.grid_x.tick_minor.label_fn : conf.grid_x.tick_major.label_fn;
						float ys = scale_max - i * sinc;

						window->DrawList->AddLine(
							ImVec2(x0, y0),
							ImVec2(x0, y1),
							color,
							thickness);

						if (label_text_size != 0.0f) {
							if (label_fn) {
								label_fn(user_data_list[0], buf, sizeof(buf), xs);
							}
							else {
								ImFormatString(buf, sizeof(buf), conf.grid_x.tick_minor.label_format, xs);
							}

							ImVec2 label_size = g.Font->CalcTextSizeA(g.FontSize * label_text_size, FLT_MAX, 0.0f, buf);
							ImVec2 label_pos = ImVec2(
								x0 - label_size.x * 0.5f > inner_bb.Min.x ? (x0 + label_size.x * 0.5f < inner_bb.Max.x ? x0 - label_size.x * 0.5f : x0 - label_size.x) : x0,
								y1 + g.Style.ItemInnerSpacing.y);
							window->DrawList->AddText(NULL, g.FontSize * label_text_size, label_pos, label_color, buf);
						}
					}
					break;
				}
				case PlotConfig::Scale::Log10: {
					// TODO: support tick labels
					float start = 1.f;
					while (start < x_max) {
						for (int i = 1; i < 10; ++i) {
							float x = start * i;
							if (x < x_min) continue;
							if (x > x_max) break;
							float t = log10(x / x_min) / log10(x_max / x_min);
							int x0 = ImLerp(inner_bb.Min.x, inner_bb.Max.x, t);
							window->DrawList->AddLine(
								ImVec2(x0, y0),
								ImVec2(x0, y1),
								(i > 1) ? col_minor : col_major,
								(i > 1) ? thickness_minor : thickness_major);
						}
						start *= 10.f;
					}
					break;
				}
				}
			}
			if (conf.grid_y.show) {
				int x0 = inner_bb.Min.x;
				int x1 = inner_bb.Max.x;

				int ticks = conf.grid_y.ticks;
				int subticks = conf.grid_y.subticks;
				float tick_size = conf.grid_y.size;

				if (ticks != -1) {
					tick_size = (scale_max - scale_min) / (ImMax(1, ticks - 1) * ImMax(1, subticks - 1));
				}
				if (conf.grid_y.sanitize_fn) {
					tick_size = conf.grid_y.sanitize_fn(user_data_list[0], conf.values.count, ticks, subticks, scale_min, scale_max);
				}
				float cnt = (scale_max - scale_min) / (tick_size / subticks);
				float inc = 1.f / cnt;
				float sinc = (tick_size / subticks);
				ImU32 col_major = conf.grid_y.tick_major.color ? conf.grid_y.tick_major.color : IM_COL32(0, 0, 0, 64);
				ImU32 col_minor = conf.grid_y.tick_minor.color ? conf.grid_y.tick_minor.color : IM_COL32(0, 0, 0, 16);
				ImU32 col_label_major = conf.grid_y.tick_major.label_color ? conf.grid_y.tick_major.label_color : IM_COL32(0, 0, 0, 64);
				ImU32 col_label_minor = conf.grid_y.tick_minor.label_color ? conf.grid_y.tick_minor.label_color : IM_COL32(0, 0, 0, 16);
				float thickness_major = conf.grid_y.tick_major.thickness;
				float thickness_minor = conf.grid_y.tick_minor.thickness;
				for (int i = 0; i <= cnt; ++i) {
					int y0 = ImLerp(inner_bb.Min.y, inner_bb.Max.y, i * inc);
					float ys = scale_max - i * sinc;

					bool is_sub = subticks > 1 && i % subticks;
					ImU32 color = is_sub ? col_minor : col_major;
					ImU32 label_color = is_sub ? col_label_minor : col_label_major;
					float thickness = is_sub ? thickness_minor : thickness_major;
					float label_text_size = is_sub ? conf.grid_y.tick_minor.label_size : conf.grid_y.tick_major.label_size;
					auto label_fn = is_sub ? conf.grid_y.tick_minor.label_fn : conf.grid_y.tick_major.label_fn;

					window->DrawList->AddLine(
						ImVec2(x0, y0),
						ImVec2(x1, y0),
						color,
						thickness);

					if (label_text_size != 0.0f) {
						if (label_fn) {
							label_fn(user_data_list[0], buf, sizeof(buf), ys);
						} else {
							ImFormatString(buf, sizeof(buf), conf.grid_x.tick_minor.label_format, ys);
						}

						ImVec2 label_size = g.Font->CalcTextSizeA(g.FontSize * label_text_size, FLT_MAX, 0.0f, buf);
						ImVec2 label_pos = ImVec2(x0 - label_size.x - g.Style.ItemInnerSpacing.x,
							y0 - label_size.y * 0.5f > inner_bb.Min.y ? (y0 + label_size.y * 0.5f < inner_bb.Max.y ? y0 - label_size.y * 0.5f : y0 - label_size.y) : y0);
						window->DrawList->AddText(NULL, g.FontSize * label_text_size, label_pos, label_color, buf);
					}
				}
			}

			const ImU32 col_hovered = GetColorU32(ImGuiCol_PlotLinesHovered);
			ImU32 col_base = GetColorU32(ImGuiCol_PlotLines);

			// draw the plots
			for (int i = 0; i < ys_count; ++i) {
				if (colors) {
					if (colors[i]) col_base = colors[i];
					else col_base = GetColorU32(ImGuiCol_PlotLines);
				}

				const size_t v0_idx = conf.values.offset;
				float v0 = ys_list ? ys_list[i][v0_idx] : ysg_list[i](user_data_list[ImMin(i, user_data_count - 1)], 0, v0_idx);
				float t0 = 0.0f;

				// Point in the normalized space of our target rectangle
				ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale));

				for (int n = 0; n < res_w; n++){
					const float t1 = t0 + t_step;
					const int v1_idx = (int)(t1 * item_count + 0.5f);
					IM_ASSERT(v1_idx >= 0 && v1_idx < conf.values.count);
					const int v1_idx_offset = conf.values.offset + (v1_idx + 1) % conf.values.count;
					const float v1 = ys_list ? ys_list[i][v1_idx_offset] : ysg_list[i](user_data_list[ImMin(i, user_data_count - 1)], v1_idx, v1_idx_offset);
					const ImVec2 tp1 = ImVec2(
						rescale(t1, x_min, x_max, conf.scale.type),
						1.0f - ImSaturate((v1 - scale_min) * inv_scale));

					// NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
					ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
					ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, tp1);

					if (v1_idx == v_hovered && conf.highlight.show && (i == 0 || !conf.highlight.only_first)) {
						window->DrawList->AddCircleFilled(pos0, conf.highlight.radius, col_hovered);
					}

					window->DrawList->AddLine(
						pos0,
						pos1,
						col_base,
						conf.line_thickness);

					t0 = t1;
					tp0 = tp1;
				}
			}

			// draw vertical lines
			if (conf.v_lines.show) {
				ImU32 color = conf.v_lines.color == 0 ? IM_COL32(0xff, 0, 0, 0x88) : conf.v_lines.color;

				for (size_t i = 0; i < conf.v_lines.count; ++i) {
					const float xs = conf.v_lines.xs ? conf.v_lines.xs[i] : x_min + (conf.v_lines.indices[i] * t_step) * (x_max - x_min);
					const float t1 = (xs - x_min) / (x_max - x_min);

					ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(t1, 0.f));
					ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(t1, 1.f));
					window->DrawList->AddLine(pos0, pos1, color);

					float label_text_size = conf.v_lines.label_size;
					auto label_fn = conf.v_lines.label_fn;
					ImU32 label_color = conf.v_lines.label_color == 0 ? color : conf.v_lines.label_color;

					if (label_fn) {
						label_fn(user_data_list[0], buf, sizeof(buf), xs);
					}
					else {
						ImFormatString(buf, sizeof(buf), conf.v_lines.label_format, xs);
					}

					ImVec2 label_size = g.Font->CalcTextSizeA(g.FontSize * label_text_size, FLT_MAX, 0.0f, buf);
					ImVec2 label_pos = ImVec2(
						(pos1.x + g.Style.ItemInnerSpacing.x + label_size.x > inner_bb.Max.x - g.Style.ItemInnerSpacing.x) ? 
							pos1.x - g.Style.ItemInnerSpacing.x - label_size.x : 
							pos1.x + g.Style.ItemInnerSpacing.x,
						pos1.y - label_size.y - g.Style.ItemInnerSpacing.y);
					window->DrawList->AddText(NULL, g.FontSize * label_text_size, label_pos, label_color, buf);
				}
			}

			// draw the selection box
			if (conf.selection.show) {
				if (hovered) {
					if (g.IO.MouseClicked[0]) {
						SetActiveID(id, window);
						FocusWindow(window);

						const int v_idx = cursor_to_idx(g.IO.MousePos, inner_bb, conf, x_min, x_max);
						uint32_t start = conf.values.offset + (v_idx % conf.values.count);
						uint32_t end = start;
						if (conf.selection.sanitize_fn)
							end = conf.selection.sanitize_fn(end - start) + start;
						if (end < conf.values.offset + conf.values.count) {
							*conf.selection.start = start;
							*conf.selection.length = end - start;
							status = PlotStatus::selection_updated;
						}
					}
				}

				if (g.ActiveId == id) {
					if (g.IO.MouseDown[0]) {
						const int v_idx = cursor_to_idx(g.IO.MousePos, inner_bb, conf, x_min, x_max);
						const uint32_t start = *conf.selection.start;
						uint32_t end = conf.values.offset + (v_idx % conf.values.count);
						if (end > start) {
							if (conf.selection.sanitize_fn)
								end = conf.selection.sanitize_fn(end - start) + start;
							if (end < conf.values.offset + conf.values.count) {
								*conf.selection.length = end - start;
								status = PlotStatus::selection_updated;
							}
						}
					}
					else {
						ClearActiveID();
					}
				}

				ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max,
					ImVec2(t_step * *conf.selection.start, 0.f));
				ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max,
					ImVec2(t_step * (*conf.selection.start + *conf.selection.length), 1.f));
				window->DrawList->AddRectFilled(pos0, pos1, IM_COL32(128, 128, 128, 32));
				window->DrawList->AddRect(pos0, pos1, IM_COL32(128, 128, 128, 128));
			}
		}

		// Text overlay
		if (conf.overlay.show) {
			RenderTextClipped(ImVec2(inner_bb.Min.x, 
				inner_bb.Min.y + ImMin(0.0f, conf.overlay.position.y) * (overlay_text_size.y + g.Style.ItemInnerSpacing.y)), 
				ImVec2(inner_bb.Max.x, inner_bb.Max.y),
				conf.overlay.text, NULL, NULL, ImVec2(conf.overlay.position.x, 0.0f));
		}

		return status;
	}

	namespace Tables
	{
		////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		int sortDelta(T const& a, T const& b)
		{
			return (a < b) ? -1 : (b < a) ? 1 : 0;
		}

		////////////////////////////////////////////////////////////////////////////////
		int sortBoolean(std::string const& a, std::string const& b, ImGuiSortDirection direction)
		{
			return direction == ImGuiSortDirection_Ascending ? sortDelta(a, b) : sortDelta(b, a);
		}

		////////////////////////////////////////////////////////////////////////////////
		int sortLexicographical(std::string const& a, std::string const& b, ImGuiSortDirection direction)
		{
			return direction == ImGuiSortDirection_Ascending ? sortDelta(a, b) : sortDelta(b, a);
		}

		////////////////////////////////////////////////////////////////////////////////
		int sortIntegral(std::string const& as, std::string const& bs, ImGuiSortDirection direction)
		{
			int a = strtol(as.c_str(), nullptr, 10), b = strtol(bs.c_str(), nullptr, 10);
			return direction == ImGuiSortDirection_Ascending ? sortDelta(a, b) : sortDelta(b, a);
		}

		////////////////////////////////////////////////////////////////////////////////
		int sortFloatingPoint(std::string const& as, std::string const& bs, ImGuiSortDirection direction)
		{
			float a = strtof(as.c_str(), nullptr), b = strtof(bs.c_str(), nullptr);
			return direction == ImGuiSortDirection_Ascending ? sortDelta(a, b) : sortDelta(b, a);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Table sort callbacks
		using SortComparator = int(*)(std::string const& as, std::string const& bs, ImGuiSortDirection direction);
		static SortComparator s_sortCmp[] =
		{
			sortBoolean,
			sortIntegral,
			sortIntegral,
			sortFloatingPoint,
			sortFloatingPoint,
			sortLexicographical
		};

		////////////////////////////////////////////////////////////////////////////////
		int sortCmp(std::string const& a, std::string const& b, TableConfig::Column::DataType data_type, 
			const ImGuiSortDirection sort_direction)
		{
			return s_sortCmp[data_type](a, b, sort_direction);
		}

		////////////////////////////////////////////////////////////////////////////////
		bool sortCallback(size_t r1, size_t r2, TableConfig const& conf, ImGuiTableSortSpecs* sort_specs)
		{
			// Traverse the list of sort specs
			for (int n = 0; n < sort_specs->SpecsCount; n++)
			{
				const int sort_col_id = sort_specs->Specs[n].ColumnIndex;

				// Data type and sort direction
				TableConfig::Column::DataType data_type = conf.cols.alignments.size() > sort_col_id ?
					conf.cols.data_types[sort_col_id] : TableConfig::Column::STRING;
				const ImGuiSortDirection sort_direction = sort_specs->Specs[n].SortDirection;

				// The two rows to sort
				std::string const& as = conf.rows.values[r1][sort_col_id];
				std::string const& bs = conf.rows.values[r2][sort_col_id];

				// Invoke the comparison function
				const int delta = sortCmp(as, bs, data_type, sort_direction);

				// Early out if we can already determine the ordering
				if (delta < 0) return true;
				if (delta > 0) return false;
			}

			// Default to false (indicates 'a==b')
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void Table(const char* label, TableConfig conf)
	{
		PushID(label);

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		ImGuiWindow* window = GetCurrentWindow();

		// Calculate the header widths
		std::vector<float> colWidths(conf.cols.count);
		for (size_t i = 0; i < conf.cols.count; ++i)
			colWidths[i] = CalcTextSize(conf.cols.headers[i]).x;
		for (size_t i = 0; i < conf.rows.values.size(); ++i)
		for (size_t j = 0; j < conf.cols.count; ++j)
			colWidths[j] = glm::max(colWidths[j], CalcTextSize(conf.rows.values[i][j].c_str()).x);

		float totalWidth = 0.0f;
		for (size_t i = 0; i < conf.cols.count; ++i)
			totalWidth += colWidths[i];

		// How many rows to display
		int tableRows = conf.rows.values.size();
		if (conf.rows.display_min)
			tableRows = ImMax(tableRows, conf.rows.display_min);
		if (conf.rows.display_max)
			tableRows = ImMin(tableRows, conf.rows.display_max);

		// Width of a row
		float rowWidth = 0.0f;
		for (size_t i = 0; i < conf.cols.count; ++i)
			rowWidth += colWidths[i];

		// Various layout-related properties
		float rowHeight = CalcTextSize(conf.cols.headers[0]).y + style.CellPadding.y * 2.0f;  // height of a single row
		float windowPadding = 2.0f * style.WindowPadding.y;                                   // window padding
		float separatorHeight = 1.0f + style.ItemSpacing.y;                                   // height of the separator line
		float headerHeight = rowHeight + separatorHeight;                                     // Height of the header
		float contentsHeight = tableRows * rowHeight;                                         // Height of the contents area
		float tableHeight = contentsHeight + headerHeight + windowPadding;                    // Height of the entire table

		// Table flags
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit |
			ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Reorderable |
			ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate | ImGuiTableFlags_Resizable;

		// Generate the table contents
		if (BeginTable("TableData", conf.cols.count, tableFlags, ImVec2(0.0f, tableHeight)))
		{
			// Setup the columns
			TableSetupScrollFreeze(0, 1); // Make top row always visible
			for (size_t i = 0; i < conf.cols.count; ++i)
			{
				const bool lastCol = i == (conf.cols.count - 1);
				const ImGuiTableFlags columnFlags = lastCol ? ImGuiTableColumnFlags_WidthStretch : ImGuiTableColumnFlags_WidthFixed;
				TableSetupColumn(conf.cols.headers[i], columnFlags, colWidths[i] + 2.0f * style.ItemSpacing.x);
			}
			TableHeadersRow();

			// Sort the rows
			std::vector<size_t> sortIndices = std::iota<size_t>(conf.rows.values.size(), 0);
			ImGuiTableSortSpecs* sorts_specs = TableGetSortSpecs();
			if (sorts_specs != nullptr && sorts_specs->SpecsCount > 0)
			{
				std::sort(sortIndices.begin(), sortIndices.end(), [&](auto const& r1, auto const& r2)
				{
					return Tables::sortCallback(r1, r2, conf, sorts_specs);
				});
			}

			// Generate the rows
			size_t newSelectedRow = conf.selection.row_id ? *conf.selection.row_id : 0;
			size_t newSelectedCol = conf.selection.col_id ? *conf.selection.col_id : 0;
			for (size_t rowId = 0; rowId < conf.rows.values.size(); ++rowId)
			{
				size_t originalRowId = sortIndices[rowId];
				for (size_t colId = 0; colId < conf.cols.count; ++colId)
				{
					// Properties of the current cell
					const char* cell_val = conf.rows.values[originalRowId][colId].c_str();
					TableConfig::Column::Alignment alignment = conf.cols.alignments.size() > colId ? 
						conf.cols.alignments[colId] : TableConfig::Column::Left;

					// Try to jump to the next column
					if (!TableNextColumn())
						continue;

					// Generate the node itself
					if (conf.selection.enabled)
					{
						bool selected = true;
						if (conf.selection.col_id != nullptr)
							selected = selected && *conf.selection.col_id == colId;
						if (conf.selection.row_id != nullptr)
							selected = selected && *conf.selection.row_id == originalRowId;
						bool prevSelected = selected;

						PushID(rowId * conf.cols.count + colId);
						ImVec2 cursorPos = GetCursorPos();
						selected = Selectable("###selection", &selected, ImGuiSelectableFlags_SpanAllColumns);
						SetCursorPos(cursorPos);
						PopID();

						if (selected && !prevSelected)
						{
							newSelectedCol = colId;
							newSelectedRow = originalRowId;
						}
					}

					// Handle the alignment
					switch (alignment)
					{
					case TableConfig::Column::Left:
						break;
					case TableConfig::Column::Center:
						SetCursorPosX(GetCursorPosX() + (colWidths[colId] - CalcTextSize(cell_val).x) * 0.5f);
						break;
					case TableConfig::Column::Right:
						SetCursorPosX(GetCursorPosX() + colWidths[colId] - CalcTextSize(cell_val).x);
						break;
					}

					// Cell label
					Text(cell_val);

					// Tooltip
					if (IsItemHovered())
						SetTooltip(cell_val);
				}

				if (conf.selection.col_id != nullptr)
					*conf.selection.col_id = newSelectedCol;
				if (conf.selection.row_id != nullptr)
					*conf.selection.row_id = newSelectedRow;
			}

			EndTable();
		}

		PopID();
	}

	////////////////////////////////////////////////////////////////////////////////
	FilterRegex::FilterRegex(std::string const& text):
		m_text(text)
	{
		update();
	}

	////////////////////////////////////////////////////////////////////////////////
	void FilterRegex::update()
	{
		m_empty = m_valid = m_text.empty();

		// Nothing to do if the source is empty
		if (m_empty)
			return;

		// Start from the raw filter source
		m_regexText = m_text;
		m_targetComponent.clear();

		// Try to match one of the components
		for (auto const& compName : m_validComponents)
		{
			if (m_regexText.substr(0, compName.length() + 3) == "$" + compName + "$:")
			{
				m_targetComponent = compName;
				m_regexText = m_regexText.substr(compName.length() + 3);
				break;
			}
		}

		// Ignore case, if needed
		if (m_ignoreCase)
		{
			std::transform(m_regexText.begin(), m_regexText.end(), m_regexText.begin(), [](auto c) { return std::tolower(c); });
		}

		// Construct the regex object
		try
		{
			m_regex = std::regex(m_regexText);
			m_valid = true;
		}
		catch (std::exception e)
		{
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	bool FilterRegex::test(std::string const& source, std::string text) const
	{
		// Are we ignoring case or not
		if (m_ignoreCase) std::transform(text.begin(), text.end(), text.begin(), [](auto c) { return std::tolower(c); });

		// Only filter in the targeted log component
		if (m_targetComponent.size() > 0 && source != m_targetComponent) return false;

		std::smatch capture;
		return std::regex_search(text, capture, m_regex);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool FilterRegex::test(std::string text) const
	{
		// Are we ignoring case or not
		if (m_ignoreCase) std::transform(text.begin(), text.end(), text.begin(), [](auto c) { return std::tolower(c); });

		std::smatch capture;
		return std::regex_search(text, capture, m_regex);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool RegexFilter(const char* label, FilterRegex& filter, ImTextureID validTexture, ImTextureID invalidTexture)
	{
		PushID(label);

		// Input text
		bool filterChanged = ImGui::InputText(label, filter.m_text);

		// Ignore case button
		SameLine();
		filterChanged = Checkbox("Ignore Case", &filter.m_ignoreCase) || filterChanged;

		// Small validity icon
		ImTextureID texture = 0;
		if (filter.m_valid && validTexture) texture = validTexture;
		if (!filter.m_valid && invalidTexture) texture = invalidTexture;

		// Generate the texture, if present
		if (texture)
		{
			SameLine();
			ImageSquare(texture, ImVec2(0, 1), ImVec2(1, 0), ImColor(255, 255, 255, 255), ImColor(0, 0, 0, 0));
		}

		if (filterChanged) filter.update();
		
		PopID();

		return filterChanged;
	}

	////////////////////////////////////////////////////////////////////////////////
	void TextAlign(const char* format, ImGuiTextAlignment alignment, ...)
	{
		va_list args;
		va_start(args, alignment);
		TextAlignV(format, alignment, args);
		va_end(args);
	}

	////////////////////////////////////////////////////////////////////////////////
	void TextAlignV(const char* format, ImGuiTextAlignment alignment, va_list args)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		const char* text_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), format, args);

		const ImVec2 textSize = CalcTextSize(g.TempBuffer, text_end);
		if (alignment == ImGuiTextAlignment_Left)
			; // do nothing
		else if (alignment == ImGuiTextAlignment_Center)
			SetCursorPosX((window->Size.x - textSize.x) * 0.5f);
		else if (alignment == ImGuiTextAlignment_Right)
			SetCursorPosX(window->Size.x - textSize.x);
		TextEx(g.TempBuffer, text_end, ImGuiTextFlags_NoWidthForLargeClippedText);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** std::string input text */
	bool InputText(const char* label, std::string& attribute, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		std::array<char, 256> stringInput;
		std::copy(attribute.begin(), attribute.end(), stringInput.begin());
		stringInput[attribute.end() - attribute.begin()] = 0;
		if (InputText(label, stringInput.data(), stringInput.size(), flags, callback, user_data))
		{
			attribute = stringInput.data();
			return true;
		}
		return false;
	};

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, int* val, std::vector<const char*> const& options)
	{
		return Combo(label, val, options.data(), (int)options.size());
	}

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, int* val, std::vector<std::string> const& options)
	{
		return Combo(label, val, std::to_cstr(options));
	}

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, std::string& val, std::vector<const char*> const& options)
	{
		int valId = std::distance(options.begin(), std::find_if(options.begin(), options.end(), [&](const char* v) { return v == val; }));
		bool changed = ImGui::Combo(label, &valId, options.data(), options.size());
		if (valId < options.size()) val = options[valId];
		return changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, std::string& val, std::vector<std::string> const& options)
	{
		return Combo(label, val, std::to_cstr(options));
	}

	////////////////////////////////////////////////////////////////////////////////
	bool RadioButton(const char* label, int* val, std::vector<const char*> const& options, bool sameLine)
	{
		int prev = *val;
		Text(label);
		for (int i = 0; i < options.size(); ++i)
		{
			if (i > 0 && sameLine) SameLine();
			RadioButton(options[i], val, i);
		}
		return *val == prev;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool RadioButton(const char* label, int* val, std::vector<std::string> const& options, bool sameLine)
	{
		return RadioButton(label, val, std::to_cstr(options), sameLine);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool InputTextPreset(const char* label, std::string& attribute, std::vector<std::string> const& options, ImGuiInputTextFlags flags)
	{
		static const std::vector<std::string> s_custom = std::vector<std::string>{ "Custom"s };

		std::vector<std::string> allOptions = options;
		allOptions.insert(allOptions.end(), s_custom.begin(), s_custom.end());
		std::vector<const char*> optionsCstr = std::to_cstr(allOptions);

		int val = std::distance(options.begin(), std::find(options.begin(), options.end(), attribute));
		bool changed = Combo(label, &val, optionsCstr.data(), (int)optionsCstr.size());

		if (changed)
		{
			attribute = (val == options.size()) ? "" : options[val];
		}

		if (val == options.size())
		{
			PushID(label + 1);

			changed = InputText(label, attribute, flags) || changed;

			PopID();
		}

		return changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool ButtonEx(const char* label, const char* referenceLabel, ImGuiButtonFlags flags)
	{
		const ImGuiStyle& style = GImGui->Style;

		ImVec2 labelSize = CalcTextSize(referenceLabel, NULL, false);
		ImVec2 referenceSize = CalcTextSize(referenceLabel, NULL, false);
		ImVec2 size = ImMax(labelSize, referenceSize) + ImVec2(style.FramePadding.x * 2.0f, style.FramePadding.y * 2.0f);

		return ButtonEx(label, size, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool CollapsingHeader(const char* label, int numControls, bool** results, const char** button_texts, ImTextureID* texture_ids, ImGuiTreeNodeFlags flags)
	{		
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiID id = window->GetID(label);
		bool is_open = TreeNodeBehavior(id, flags | ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_AllowItemOverlap, label);

		// Context object
		ImGuiContext& context = *GImGui;

		// Hover data backup
		ImGuiLastItemDataBackup last_item_backup;

		// Extract the previous cursor pos and padding
		ImVec2 prev_cursor_pos = GetCursorScreenPos();
		float prev_padding = context.Style.FramePadding.y;

		// Rect of the collapsing header
		ImRect header_rect = window->DC.LastItemRect;

		// Size of a label
		ImVec2 empty_label_size = CalcTextSize("", NULL, true);

		// Set the new padding size
		context.Style.FramePadding.y = -context.Style.FramePadding.y * 0.5f;

		// Push colors for the close button
		PushStyleColor(ImGuiCol_ButtonActive, context.Style.Colors[ImGuiCol_FrameBgActive]);
		PushStyleColor(ImGuiCol_ButtonHovered, context.Style.Colors[ImGuiCol_FrameBgHovered]);
		PushStyleColor(ImGuiCol_Button, context.Style.Colors[ImGuiCol_FrameBg]);

		// Current draw pos (x of the lower left corner and y of the center
		ImVec2 drawPos = ImVec2(header_rect.Max.x, header_rect.GetCenter().y);

		// Create header controls
		for (int i = 0; i < numControls; ++i)
		{
			// Create a regular button, with text
			if (button_texts[i] != nullptr)
			{
				const ImVec2 label_size = CalcTextSize(button_texts[i], NULL, true);

				ImVec2 button_size = ImVec2(label_size.x + context.Style.FramePadding.x * 2, label_size.y + context.Style.FramePadding.y * 2);
				ImVec2 button_pos = ImVec2(ImMin(drawPos.x, window->ClipRect.Max.x) - context.Style.FramePadding.x - button_size.x, drawPos.y - button_size.y / 2);
				SetCursorScreenPos(button_pos);
				*results[i] = Button(button_texts[i]);
			}

			// Create a textured button
			else if (texture_ids[i] != nullptr)
			{
				ImVec2 button_size = ImVec2(empty_label_size.y + context.Style.FramePadding.y * 2, empty_label_size.y + context.Style.FramePadding.y * 2);
				ImVec2 button_pos = ImVec2(ImMin(drawPos.x, window->ClipRect.Max.x) - context.Style.FramePadding.x - button_size.x, drawPos.y - button_size.y / 2);
				SetCursorScreenPos(button_pos);
				*results[i] = ImageButton(texture_ids[i], button_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), 0.0f, ImVec4(0, 0, 0, 0), context.Style.Colors[ImGuiCol_CheckMark]);
			}

			// Create a checkbox
			else
			{
				// Create the enable checkbox
				ImVec2 button_size = ImVec2(empty_label_size.y + context.Style.FramePadding.y * 2, empty_label_size.y + context.Style.FramePadding.y * 2);
				ImVec2 button_pos = ImVec2(ImMin(drawPos.x, window->ClipRect.Max.x) - context.Style.FramePadding.x - button_size.y, drawPos.y - button_size.y / 2);
				SetCursorScreenPos(button_pos);
				Checkbox("", results[i]);
			}

			// Update the draw position
			drawPos.x = window->DC.LastItemRect.Min.x;
		}

		// Pop the close button colors
		PopStyleColor();
		PopStyleColor();
		PopStyleColor();

		// Reset the cursor position and padding
		SetCursorScreenPos(prev_cursor_pos);
		context.Style.FramePadding.y = prev_padding;

		// Restore hover data
		last_item_backup.Restore();

		return is_open;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool SelectableWithIcon(const char* label, bool item_selected, ImTextureID user_texture_id, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		bool value_changed = false;

		ImVec2 textSize = ImGui::CalcTextSize(label);
		if (Selectable("", item_selected, 0, ImVec2(0.0f, textSize.y)))
			value_changed = true;

		SameLineWithinComposite();
		ImageSquare(user_texture_id, uv0, uv1, tint_col, border_col);
		SameLineWithinComposite();
		Text(label);

		return value_changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	void ImageSquare(ImTextureID user_texture_id, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		// Height of the image
		float iconHeight = CalcTextSize("XX").y;
		ImVec2 iconSize = ImVec2(iconHeight, iconHeight);

		// Generate the image
		Image(user_texture_id, iconSize, uv0, uv1, tint_col, border_col);
	}

	////////////////////////////////////////////////////////////////////////////////
	float applyDpiScale(float f, float dpiScale)
	{
		return glm::floor(f * dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	ImVec2 applyDpiScale(ImVec2 v, float dpiScale)
	{
		return ImVec2(applyDpiScale(v.x, dpiScale), applyDpiScale(v.y, dpiScale));
	}

	////////////////////////////////////////////////////////////////////////////////
	void PatchStyleSizesHighRes(ImGuiStyle* dst, float dpiScale, bool respectThemeValues = true)
	{
		ImGuiStyle* style = dst ? dst : &GetStyle();

		// Layout
		style->WindowPadding = applyDpiScale(ImVec2(15.0f, 15.0f), dpiScale);
		style->FramePadding = applyDpiScale(ImVec2(8.0f, 4.0f), dpiScale);
		style->ItemSpacing = applyDpiScale(ImVec2(12.0f, 5.0f), dpiScale);
		style->ItemInnerSpacing = applyDpiScale(ImVec2(8.0f, 6.0f), dpiScale);
		style->IndentSpacing = applyDpiScale(25.0f, dpiScale);
		style->CellPadding = applyDpiScale(ImVec2(8.0f, 4.0f), dpiScale);

		// Size
		style->ScrollbarSize = applyDpiScale(20.0f, dpiScale);
		style->GrabMinSize = applyDpiScale(20.0f, dpiScale);

		// Rounding
		style->FrameRounding = applyDpiScale(4.0f, dpiScale);
		style->GrabRounding = applyDpiScale(4.0f, dpiScale);
		style->ScrollbarRounding = applyDpiScale(12.0f, dpiScale);
		style->TabRounding = applyDpiScale(4.0f, dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void PatchStyleSizesLowRes(ImGuiStyle* dst, float dpiScale, bool respectThemeValues = true)
	{
		ImGuiStyle* style = dst ? dst : &GetStyle();

		// Layout
		style->WindowPadding = applyDpiScale(ImVec2(6.0f, 6.0f), dpiScale);
		style->FramePadding = applyDpiScale(ImVec2(4.0f, 2.0f), dpiScale);
		style->ItemSpacing = applyDpiScale(ImVec2(4.0f, 2.0f), dpiScale);
		style->ItemInnerSpacing = applyDpiScale(ImVec2(4.0f, 6.0f), dpiScale);
		style->IndentSpacing = applyDpiScale(25.0f, dpiScale);
		style->CellPadding = applyDpiScale(ImVec2(4.0f, 2.0f), dpiScale);

		// Size
		style->ScrollbarSize = applyDpiScale(12.0f, dpiScale);
		style->GrabMinSize = applyDpiScale(20.0f, dpiScale);

		// Rounding
		style->FrameRounding = applyDpiScale(3.0f, dpiScale);
		style->GrabRounding = applyDpiScale(4.0f, dpiScale);
		style->ScrollbarRounding = applyDpiScale(4.0f, dpiScale);
		style->TabRounding = applyDpiScale(4.0f, dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void PatchStyleSizes(bool lowres, ImGuiStyle* dst, float dpiScale, bool respectThemeValues = true)
	{
		if (lowres)
			PatchStyleSizesLowRes(dst, dpiScale);
		else
			PatchStyleSizesHighRes(dst, dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCustomClassic(ImGuiStyle* dst, float dpiScale, bool lowres)
	{
		ImGuiStyle* style = dst ? dst : &GetStyle();
		ImVec4* colors = style->Colors;

		colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
		colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
		colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
		colors[ImGuiCol_Button] = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
		colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
		colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
		colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
		colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
		colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_Header] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

		PatchStyleSizes(lowres, dst, dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCustomLight(ImGuiStyle * dst, float dpiScale, bool lowres)
	{
		ImGuiStyle* style = dst ? dst : &GetStyle();
		ImVec4* colors = style->Colors;

		colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
		colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
		colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.90f);
		colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
		colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
		colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_Header] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

		PatchStyleSizes(lowres, dst, dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCustomDark(ImGuiStyle * dst, float dpiScale, bool lowres)
	{
		ImGuiStyle* style = dst ? dst : &GetStyle();
		ImVec4* colors = style->Colors;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
		colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
		colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
		colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

		PatchStyleSizes(lowres, dst, dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsIntelliJDracula(ImGuiStyle* dst, float dpiScale)
	{
		auto *style = (dst ? dst : &GetStyle());

		style->WindowRounding = 5.3f;
		style->GrabRounding = style->FrameRounding = 2.3f;
		style->ScrollbarRounding = 5.0f;
		style->FrameBorderSize = 1.0f;
		style->ItemSpacing.y = 6.5f;

		style->Colors[ImGuiCol_Text] = { 0.73333335f, 0.73333335f, 0.73333335f, 1.00f };
		style->Colors[ImGuiCol_TextDisabled] = { 0.34509805f, 0.34509805f, 0.34509805f, 1.00f };
		style->Colors[ImGuiCol_WindowBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.94f };
		style->Colors[ImGuiCol_ChildBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.00f };
		style->Colors[ImGuiCol_PopupBg] = { 0.23529413f, 0.24705884f, 0.25490198f, 0.94f };
		style->Colors[ImGuiCol_Border] = { 0.33333334f, 0.33333334f, 0.33333334f, 0.50f };
		style->Colors[ImGuiCol_BorderShadow] = { 0.15686275f, 0.15686275f, 0.15686275f, 0.00f };
		style->Colors[ImGuiCol_FrameBg] = { 0.16862746f, 0.16862746f, 0.16862746f, 0.54f };
		style->Colors[ImGuiCol_FrameBgHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
		style->Colors[ImGuiCol_FrameBgActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
		style->Colors[ImGuiCol_TitleBg] = { 0.04f, 0.04f, 0.04f, 1.00f };
		style->Colors[ImGuiCol_TitleBgCollapsed] = { 0.16f, 0.29f, 0.48f, 1.00f };
		style->Colors[ImGuiCol_TitleBgActive] = { 0.00f, 0.00f, 0.00f, 0.51f };
		style->Colors[ImGuiCol_MenuBarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.80f };
		style->Colors[ImGuiCol_ScrollbarBg] = { 0.27058825f, 0.28627452f, 0.2901961f, 0.60f };
		style->Colors[ImGuiCol_ScrollbarGrab] = { 0.21960786f, 0.30980393f, 0.41960788f, 0.51f };
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
		style->Colors[ImGuiCol_ScrollbarGrabActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 0.91f };
		// style->Colors[ImGuiCol_ComboBg]               = {0.1f, 0.1f, 0.1f, 0.99f};
		style->Colors[ImGuiCol_CheckMark] = { 0.90f, 0.90f, 0.90f, 0.83f };
		style->Colors[ImGuiCol_SliderGrab] = { 0.70f, 0.70f, 0.70f, 0.62f };
		style->Colors[ImGuiCol_SliderGrabActive] = { 0.30f, 0.30f, 0.30f, 0.84f };
		style->Colors[ImGuiCol_Button] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.49f };
		style->Colors[ImGuiCol_ButtonHovered] = { 0.21960786f, 0.30980393f, 0.41960788f, 1.00f };
		style->Colors[ImGuiCol_ButtonActive] = { 0.13725491f, 0.19215688f, 0.2627451f, 1.00f };
		style->Colors[ImGuiCol_Header] = { 0.33333334f, 0.3529412f, 0.36078432f, 0.53f };
		style->Colors[ImGuiCol_HeaderHovered] = { 0.453125f, 0.67578125f, 0.99609375f, 0.67f };
		style->Colors[ImGuiCol_HeaderActive] = { 0.47058827f, 0.47058827f, 0.47058827f, 0.67f };
		style->Colors[ImGuiCol_Separator] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
		style->Colors[ImGuiCol_SeparatorHovered] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
		style->Colors[ImGuiCol_SeparatorActive] = { 0.31640625f, 0.31640625f, 0.31640625f, 1.00f };
		style->Colors[ImGuiCol_ResizeGrip] = { 1.00f, 1.00f, 1.00f, 0.85f };
		style->Colors[ImGuiCol_ResizeGripHovered] = { 1.00f, 1.00f, 1.00f, 0.60f };
		style->Colors[ImGuiCol_ResizeGripActive] = { 1.00f, 1.00f, 1.00f, 0.90f };
		style->Colors[ImGuiCol_PlotLines] = { 0.61f, 0.61f, 0.61f, 1.00f };
		style->Colors[ImGuiCol_PlotLinesHovered] = { 1.00f, 0.43f, 0.35f, 1.00f };
		style->Colors[ImGuiCol_PlotHistogram] = { 0.90f, 0.70f, 0.00f, 1.00f };
		style->Colors[ImGuiCol_PlotHistogramHovered] = { 1.00f, 0.60f, 0.00f, 1.00f };
		style->Colors[ImGuiCol_TextSelectedBg] = { 0.18431373f, 0.39607847f, 0.79215693f, 0.90f };
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsMaya(ImGuiStyle* dst, float dpiScale)
	{
		auto *style = (dst ? dst : &GetStyle());

		//style->ChildWindowRounding = 3.f;
		style->GrabRounding = 0.f;
		style->WindowRounding = 0.f;
		style->ScrollbarRounding = 3.f;
		style->FrameRounding = 3.f;
		style->WindowTitleAlign = ImVec2(0.5f, 0.5f);

		style->Colors[ImGuiCol_Text] = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
		style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style->Colors[ImGuiCol_WindowBg] = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
		style->Colors[ImGuiCol_ChildBg] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		style->Colors[ImGuiCol_PopupBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		style->Colors[ImGuiCol_Border] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		style->Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		style->Colors[ImGuiCol_TitleBg] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		//style->Colors[ImGuiCol_ComboBg] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
		style->Colors[ImGuiCol_CheckMark] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
		style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
		style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
		style->Colors[ImGuiCol_Button] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
		style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
		style->Colors[ImGuiCol_Header] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		//style->Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		//style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
		//style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
		style->Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.32f, 0.52f, 0.65f, 1.00f);
		style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsMicrosoftLight(ImGuiStyle* dst, float dpiScale)
	{
		auto *style = (dst ? dst : &GetStyle());

		int hspacing = 8;
		int vspacing = 6;
		style->DisplaySafeAreaPadding = ImVec2(0, 0);
		style->WindowPadding = ImVec2(hspacing / 2, vspacing);
		style->FramePadding = ImVec2(hspacing, vspacing);
		style->ItemSpacing = ImVec2(hspacing, vspacing);
		style->ItemInnerSpacing = ImVec2(hspacing, vspacing);
		style->IndentSpacing = 20.0f;

		style->WindowRounding = 0.0f;
		style->FrameRounding = 0.0f;

		style->WindowBorderSize = 0.0f;
		style->FrameBorderSize = 1.0f;
		style->PopupBorderSize = 1.0f;

		style->ScrollbarSize = 20.0f;
		style->ScrollbarRounding = 0.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 0.0f;

		ImVec4 white = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		ImVec4 transparent = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		ImVec4 dark = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
		ImVec4 darker = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);

		ImVec4 background = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
		ImVec4 text = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		ImVec4 border = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		ImVec4 grab = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
		ImVec4 header = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		ImVec4 active = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
		ImVec4 hover = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);
		ImVec4 tab = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);

		style->Colors[ImGuiCol_Text] = text;
		style->Colors[ImGuiCol_WindowBg] = background;
		style->Colors[ImGuiCol_ChildBg] = background;
		style->Colors[ImGuiCol_PopupBg] = white;

		style->Colors[ImGuiCol_TitleBg] = header;
		style->Colors[ImGuiCol_TitleBgCollapsed] = header;
		style->Colors[ImGuiCol_TitleBgActive] = header;

		style->Colors[ImGuiCol_Border] = border;
		style->Colors[ImGuiCol_BorderShadow] = transparent;

		style->Colors[ImGuiCol_Button] = header;
		style->Colors[ImGuiCol_ButtonHovered] = hover;
		style->Colors[ImGuiCol_ButtonActive] = active;

		style->Colors[ImGuiCol_FrameBg] = white;
		style->Colors[ImGuiCol_FrameBgHovered] = hover;
		style->Colors[ImGuiCol_FrameBgActive] = active;

		style->Colors[ImGuiCol_MenuBarBg] = header;
		style->Colors[ImGuiCol_Header] = header;
		style->Colors[ImGuiCol_HeaderHovered] = hover;
		style->Colors[ImGuiCol_HeaderActive] = active;

		style->Colors[ImGuiCol_CheckMark] = text;
		style->Colors[ImGuiCol_SliderGrab] = grab;
		style->Colors[ImGuiCol_SliderGrabActive] = darker;

		//style->Colors[ImGuiCol_CloseButton] = transparent;
		//style->Colors[ImGuiCol_CloseButtonHovered] = transparent;
		//style->Colors[ImGuiCol_CloseButtonActive] = transparent;

		style->Colors[ImGuiCol_ScrollbarBg] = header;
		style->Colors[ImGuiCol_ScrollbarGrab] = grab;
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = dark;
		style->Colors[ImGuiCol_ScrollbarGrabActive] = darker;

		style->Colors[ImGuiCol_Separator] = header;
		style->Colors[ImGuiCol_SeparatorHovered] = header;
		style->Colors[ImGuiCol_SeparatorActive] = header;

		style->Colors[ImGuiCol_ResizeGrip] = header;
		style->Colors[ImGuiCol_ResizeGripHovered] = header;
		style->Colors[ImGuiCol_ResizeGripActive] = header;
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsUnreal(ImGuiStyle* dst, float dpiScale, bool lowres)
	{
		auto *style = (dst ? dst : &GetStyle());

		style->Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
		style->Colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		style->Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		style->Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style->Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
		style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
		style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
		style->Colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style->Colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		style->Colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
		style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
		style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
		style->Colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
		style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
		style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
		style->Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
		style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
		style->Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
		style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.60f, 0.60f, 0.60f, 0.90f);
		style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style->Colors[ImGuiCol_Tab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style->Colors[ImGuiCol_TabHovered] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style->Colors[ImGuiCol_TabActive] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f); 
		style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); 
		style->Colors[ImGuiCol_DockingPreview] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style->Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

		PatchStyleSizes(lowres, dst, dpiScale);
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsExtasy(ImGuiStyle* dst, float dpiScale)
	{
		auto *style = (dst ? dst : &GetStyle());

		style->WindowPadding = ImVec2(15, 15);
		style->WindowRounding = 5.0f;
		style->FramePadding = ImVec2(5, 5);
		style->FrameRounding = 4.0f;
		style->ItemSpacing = ImVec2(12, 8);
		style->ItemInnerSpacing = ImVec2(8, 6);
		style->IndentSpacing = 25.0f;
		style->ScrollbarSize = 15.0f;
		style->ScrollbarRounding = 9.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 3.0f;

		style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		//style->Colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
		style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_Separator] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		//style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
		//style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
		//style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
		style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsPhotoshop(ImGuiStyle* dst, float dpiScale)
	{
		auto* style = (dst ? dst : &GetStyle());
		ImVec4* colors = style->Colors;

		colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
		colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
		colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

		style->ChildRounding = 4.0f;
		style->FrameBorderSize = 1.0f;
		style->FrameRounding = 2.0f;
		style->GrabMinSize = 7.0f;
		style->PopupRounding = 2.0f;
		style->ScrollbarRounding = 12.0f;
		style->ScrollbarSize = 13.0f;
		style->TabBorderSize = 1.0f;
		style->TabRounding = 0.0f;
		style->WindowRounding = 4.0f;
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCorporateGrey(ImGuiStyle* dst, float dpiScale, bool is3D)
	{
		auto* style = (dst ? dst : &GetStyle());
		ImVec4* colors = style->Colors;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
		colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
		colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

		style->PopupRounding = 3;

		style->WindowPadding = ImVec2(4, 4);
		style->FramePadding = ImVec2(6, 4);
		style->ItemSpacing = ImVec2(6, 2);

		style->ScrollbarSize = 18;

		style->WindowBorderSize = 1;
		style->ChildBorderSize = 1;
		style->PopupBorderSize = 1;
		style->FrameBorderSize = is3D ? 1 : 0;

		style->WindowRounding = 3;
		style->ChildRounding = 3;
		style->FrameRounding = 3;
		style->ScrollbarRounding = 2;
		style->GrabRounding = 3;

		style->TabBorderSize = is3D ? 1 : 0;
		style->TabRounding = 3;

		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style->WindowRounding = 0.0f;
			style->Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsRudaDark(ImGuiStyle* dst, float dpiScale)
	{
		auto* style = (dst ? dst : &GetStyle());
		ImVec4* colors = style->Colors;

		style->FrameRounding = 4.0f;
		style->GrabRounding = 4.0f;

		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

	////////////////////////////////////////////////////////////////////////////////
	using StyleFn = std::function<void(ImGuiStyle* dst, float dpiScale)>;
	static std::unordered_map<std::string, StyleFn> s_styles =
	{
		{ "Classic",                    [](ImGuiStyle* dst, float dpiScale) { StyleColorsClassic(dst); } },
		{ "Dark",                       [](ImGuiStyle* dst, float dpiScale) { StyleColorsDark(dst); } },
		{ "Light",                      [](ImGuiStyle* dst, float dpiScale) { StyleColorsLight(dst); } },
		{ "Custom Classic",             [](ImGuiStyle* dst, float dpiScale) { StyleColorsCustomClassic(dst, dpiScale, false); } },
		{ "Custom Dark",                [](ImGuiStyle* dst, float dpiScale) { StyleColorsCustomDark(dst, dpiScale, false); } },
		{ "Custom Light",               [](ImGuiStyle* dst, float dpiScale) { StyleColorsCustomLight(dst, dpiScale, false); } },
		{ "Custom Classic (Low Res)",   [](ImGuiStyle* dst, float dpiScale) { StyleColorsCustomClassic(dst, dpiScale, true); } },
		{ "Custom Dark (Low Res)",      [](ImGuiStyle* dst, float dpiScale) { StyleColorsCustomDark(dst, dpiScale, true); } },
		{ "Custom Light (Low Res)",     [](ImGuiStyle* dst, float dpiScale) { StyleColorsCustomLight(dst, dpiScale, true); } },
		{ "IntelliJ Dracula",           [](ImGuiStyle* dst, float dpiScale) { StyleColorsIntelliJDracula(dst, dpiScale); } },
		{ "Maya",                       [](ImGuiStyle* dst, float dpiScale) { StyleColorsMaya(dst, dpiScale); } },
		{ "Microsoft",                  [](ImGuiStyle* dst, float dpiScale) { StyleColorsMicrosoftLight(dst, dpiScale); } },
		{ "Unreal",                     [](ImGuiStyle* dst, float dpiScale) { StyleColorsUnreal(dst, dpiScale, false); } },
		{ "Unreal (Low Res)",           [](ImGuiStyle* dst, float dpiScale) { StyleColorsUnreal(dst, dpiScale, true); } },
		{ "Extasy",                     [](ImGuiStyle* dst, float dpiScale) { StyleColorsExtasy(dst, dpiScale); } },
		{ "Photoshop",                  [](ImGuiStyle* dst, float dpiScale) { StyleColorsPhotoshop(dst, dpiScale); } },
		{ "Corporate Grey",             [](ImGuiStyle* dst, float dpiScale) { StyleColorsCorporateGrey(dst, dpiScale, true); } },
		{ "Corporate Grey (Flat)",      [](ImGuiStyle* dst, float dpiScale) { StyleColorsCorporateGrey(dst, dpiScale, false); } },
		{ "Ruda Dark",                  [](ImGuiStyle* dst, float dpiScale) { StyleColorsRudaDark(dst, dpiScale); } },
	};

	////////////////////////////////////////////////////////////////////////////////
	int GetNumStyles()
	{
		return s_styles.size();
	}

	////////////////////////////////////////////////////////////////////////////////
	const char* StyleColorsById(int id, ImGuiStyle* dst, float dpiScale)
	{
		assert(id < GetNumStyles());
		
		auto styleIt = s_styles.begin();
		std::advance(styleIt, id);

		// Invoke the generator
		styleIt->second(dst, dpiScale);

		// Return the name
		return styleIt->first.c_str();
	}
}