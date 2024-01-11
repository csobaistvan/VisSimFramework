#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Core/Constants.h"


////////////////////////////////////////////////////////////////////////////////
// Flags for TextAlign
enum ImGuiTextAlignment
{
	ImGuiTextAlignment_None = 0,
	ImGuiTextAlignment_Left = 1 << 0,     // Align text to the left
	ImGuiTextAlignment_Center = 1 << 1,   // Center the text
	ImGuiTextAlignment_Right = 1 << 2     // Align text to the right
};

////////////////////////////////////////////////////////////////////////////////
/// IMGUI EXTENSIONS
////////////////////////////////////////////////////////////////////////////////
namespace ImGui
{
	////////////////////////////////////////////////////////////////////////////////
	float GetFrameWidth();

	////////////////////////////////////////////////////////////////////////////////
	float GetFrameWidthWithSpacing();

	////////////////////////////////////////////////////////////////////////////////
	void SameLineWithinComposite();

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorConvertRGBtoHSV(ImVec4 rgb);

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorConvertHSVtoRGB(ImVec4 hsv);

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorFromGlmVector(glm::vec3 v);

	////////////////////////////////////////////////////////////////////////////////
	ImVec4 ColorFromGlmVector(glm::vec4 v);

	////////////////////////////////////////////////////////////////////////////////
	ImU32 Color32FromGlmVector(glm::vec3 v);

	////////////////////////////////////////////////////////////////////////////////
	ImU32 Color32FromGlmVector(glm::vec4 v);

	////////////////////////////////////////////////////////////////////////////////
	ImGuiID CurrentTabBarId();

	////////////////////////////////////////////////////////////////////////////////
	const char* CurrentTabItemName();

	////////////////////////////////////////////////////////////////////////////////
	bool BeginTabItem(const char* label, const char* active_tab_name, ImGuiTabItemFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool TreeNodeEx(const char* label, bool* popen, ImGuiTreeNodeFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool SliderFloatN(const char* label, float* v, int components, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatN(const char* label, float* v, int components, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange2(const char* label, float* vs, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* const* formats = NULL, ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange3(const char* label, float* v1, float* v2, float* v3, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* f1 = "%.3f", const char* f2 = NULL, const char* f3 = NULL, ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange3(const char* label, float* vs, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* const* formats = NULL, ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange4(const char* label, float* v1, float* v2, float* v3, float* v4, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* f1 = "%.3f", const char* f2 = NULL, const char* f3 = NULL, const char* f4 = NULL, ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRange4(const char* label, float* vs, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* const* formats = NULL, ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatRangeN(const char* label, int n, float* vs, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* const* formats = NULL, ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngle(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f deg", ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngle2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f deg", ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngle3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f deg", ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragFloatAngleN(const char* label, float* v, int components, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f deg", ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	bool DragComplex(const char* label, std::complex<float>* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	// Use this structure to pass the plot data and settings into the Plot function
	struct PlotConfig 
	{
		// Value generator function
		using value_generator_fn = float (*)(const void* data, int idx, float x);

		// Tooltip generator function
		using tooltip_generator_fn = void (*)(const void* data, int idx, float x, float y);

		// Value color generator fn
		using value_color_fn = ImU32(*)(const void* data, int idx, float x, float y, float px, float py, ImU32 col);

		// Tick size sanitize function
		using tick_size_sanitize_fn = float(*)(const void* data, int num_x, int num_ticks, int num_sub_ticks, float scale_min, float scale_max);

		// Tick label function
		using tick_label_fn = void(*)(const void* data, char* buffer, int buf_size, float val);

		// Tick color function
		using tick_color_fn = ImU32(*)(const void* data, float x, float y, float px, float py, ImU32 col);

		// Selection sanitize function
		using selection_sanitize_fn = uint32_t(*)(uint32_t);
		
		struct Values 
		{
			enum Type
			{
				Lines,
				Dots,
			};

			// type of the plot
			Type type = Lines;
			// type of each plot for multiple plots
			const Type* types = nullptr;
			// if necessary, you can provide x-axis values
			const float* xs = nullptr;
			// same thing, but with a value generator callback
			value_generator_fn xsg = nullptr;
			// array of y values. If null, use ys_list (below)
			const float* ys = nullptr;
			// same thing, but with a value generator callback
			value_generator_fn ysg = nullptr;
			// User data for the generator
			const void* user_data = nullptr;
			// the number of values in each array
			int count;
			// at which offset to start plotting.
			// Warning: count+offset must be <= length of array!
			int offset = 0;
			// plot color; if 0, use ImGuiCol_PlotLines.
			ImU32 color = 0;
			// plot color generator fn
			value_color_fn color_fn = nullptr;

			// in case you need to draw multiple plots at once, use this instead of ys
			const float** ys_list = nullptr;
			// same thing, but using a list of y generators
			const value_generator_fn* ysg_list = nullptr;
			// User data for the generator(s)
			const void** user_data_list = nullptr;
			// the number of plots to draw
			int ys_count = 0;
			// colors for each plot
			const ImU32* colors = nullptr;
			// color generators for each plot
			const value_color_fn* color_fns = nullptr;
		} values;

		struct Scale
		{
			// Minimum plot value
			float min;
			// Maximum plot value
			float max;
			// Whether we should automatically deduce the limits or not
			bool auto_min = false;
			bool auto_max = false;

			enum Type
			{
				Linear,
				Log10,
			};
			// How to scale the x-axis
			Type type = Linear;
		} scale;

		struct Highlight
		{
			// whether to show the highlight marker or not
			bool show = true;
			// whether only the first one should be highlighted or not;
			// useful when secondary lines only contain average values, etc.
			bool only_first = false;
			// Radius of the highlight marker
			float radius = 4.0f;
		} highlight;

		struct Tooltip
		{
			// should the tooltip be shown or not
			bool show = false;
			// tooltip format
			const char* format = "%g: %8.4g";
			// custom tooltip generator callback
			tooltip_generator_fn generator = nullptr;
		} tooltip;

		struct Grid
		{
			// Tick settings
			struct Tick
			{
				// color of the tick
				ImU32 color = 0;
				// color generator of the tick
				tick_color_fn color_fn = nullptr;
				// thickness of the corresponding tick line
				float thickness = 1.0f;
				// color of the label
				ImU32 label_color = 0;
				// label text format
				const char* label_format = "%3.2f";
				// label text function
				tick_label_fn label_fn = nullptr;
				// label font size
				float label_size = 0.0f;
			} tick_major, tick_minor;
			// show we draw the grid or not
			bool show = false;
			// how many ticks to draw
			float size = 100; // at which intervals to draw the grid
			// How many ticks should we draw
			int ticks = -1; // -1 means auto-compute
			// how many subticks to draw
			int subticks = 10; // how many subticks in each tick
			// tick size sanitize function; can be used to auto-determine
			// the tick size based on the current scale min/max
			tick_size_sanitize_fn sanitize_fn = nullptr;
		} grid_x, grid_y;

		struct Selection
		{
			// should we draw selections or not
			bool show = false;
			// start index for the selection window
			uint32_t* start = nullptr;
			// length of the selection window
			uint32_t* length = nullptr;
			// "Sanitize" function. Useful for FFT, where selection must be
			// of power of two
			selection_sanitize_fn sanitize_fn = nullptr;
		} selection;

		struct VerticalLines
		{
			// should we draw the vertical line or not
			bool show = false;
			// at which indices to draw the lines
			const size_t* indices = nullptr;
			// x values at which to draw the lines
			const float* xs = nullptr;
			// number of vertical lines
			size_t count = 0;
			// color of the vertical lines
			ImU32 color = 0;

			// should we also append a label or not
			bool label = false;
			// color of the label
			ImU32 label_color = 0;
			// label text format
			const char* label_format = "%3.2f";
			// label text function
			tick_label_fn label_fn = nullptr;
			// label font size
			float label_size = 0.0f;
		} v_lines;

		struct Overlay
		{
			// whether the overlay should be shown or not
			bool show = false;
			// overlay text to show
			const char* text = nullptr;
			// extra scale factor (UNSUPPORTED)
			float scale = 1.0;
			// relative positioning of the overlay text
			// x: [0.0 .. 1.0]
			// y: [-1.0 .. 1.0]
			ImVec2 position = ImVec2(0.5f, 1.0f);
		} overlay;

		ImVec2 frame_size = ImVec2(0.0f, 0.0f);
		float line_thickness = 1.0f;
		bool skip_small_lines = true;
	};

	////////////////////////////////////////////////////////////////////////////////
	enum class PlotStatus
	{
		nothing,
		selection_updated,
	};

	////////////////////////////////////////////////////////////////////////////////
	// Source: https://github.com/soulthreads/imgui-plot
	PlotStatus PlotEx(const char* label, const PlotConfig& conf);

	////////////////////////////////////////////////////////////////////////////////
	struct TableConfig
	{
		// Cell data generator
		using cell_generator = void(*)(void* user_data, uint32_t col_id, uint32_t row_id, void* buffer);

		struct Column
		{
			enum Alignment
			{
				Left,
				Center,
				Right
			};

			enum DataType
			{
				BOOL,
				INT,
				UINT,
				FLOAT,
				DOUBLE,
				STRING,
				COUNT
			};

			// number of columns in total
			uint32_t count = 0;
			// header texts
			std::vector<const char*> headers;
			// tooltip texts
			std::vector<const char*> tooltips;
			// Alignments
			std::vector<Alignment> alignments;
			// Data types
			std::vector<DataType> data_types;
		} cols;

		struct Rows
		{
			// number of rows, in total
			uint32_t count = 0;
			// the data generators
			cell_generator const* generators = nullptr;
			// data entries
			std::vector<std::vector<std::string>> values;
			// how many rows to display at minimum
			int display_min = 0;
			// how many rows to display at maximum
			int display_max = 0;
		} rows;

		struct Selection
		{
			// whether we should be listening to selection events or not
			bool enabled = false;
			// index of the selected column
			int* col_id = nullptr;
			// index of the selected row
			int* row_id = nullptr;
		} selection;
	};

	////////////////////////////////////////////////////////////////////////////////
	void Table(const char* label, TableConfig conf);

	////////////////////////////////////////////////////////////////////////////////
	// Input values for a regex
	struct FilterRegex
	{
		// Ctor
		FilterRegex(std::string const& text = std::string());

		// Update the internal structure using the raw source text
		void update();

		// Test
		bool test(std::string const& source, std::string text) const;
		bool test(std::string text) const;

		// Source text of the regex
		std::string m_text;

		// Should we ignore case or not
		bool m_ignoreCase = true;

		// -------------- Internal state ------------

		// Is the regex empty or not
		bool m_empty = true;

		// Is the regex valid or not
		bool m_valid = true;

		// The actual regex test; original text stripped of its component
		std::string m_regexText;

		// All the valid component names
		std::vector<std::string> m_validComponents;

		// Which component of the log entry to apply to 
		std::string m_targetComponent;

		// Actual regex object
		std::regex m_regex;
	};

	////////////////////////////////////////////////////////////////////////////////
	bool RegexFilter(const char* label, FilterRegex& filter, ImTextureID validTexture = 0, ImTextureID invalidTexture = 0);

	////////////////////////////////////////////////////////////////////////////////
	void Arrow(float size, ImGuiDir_ direction);

	////////////////////////////////////////////////////////////////////////////////
	void FancyText(std::string const& text);

	////////////////////////////////////////////////////////////////////////////////
	void TextAlign(const char* format, ImGuiTextAlignment alignment, ...);

	////////////////////////////////////////////////////////////////////////////////
	void TextAlignV(const char* format, ImGuiTextAlignment alignment, va_list args);

	////////////////////////////////////////////////////////////////////////////////
	/** std::string input text */
	bool InputText(const char* label, std::string& attribute, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);

	////////////////////////////////////////////////////////////////////////////////
	bool InputTextPreset(const char* label, std::string& attribute, std::vector<std::string> const& options, ImGuiInputTextFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	bool InputTextPreset(const char* label, std::string& attribute, std::unordered_map<std::string, T> const& options, ImGuiInputTextFlags flags = 0)
	{
		std::vector<std::string> optionNames(options.size());
		std::transform(options.begin(), options.end(), optionNames.begin(), std::pair_first);
		return InputTextPreset(label, attribute, optionNames, flags);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, int* val, std::vector<const char*> const& options);

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, int* val, std::vector<std::string> const& options);

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, std::string& val, std::vector<const char*> const& options);

	////////////////////////////////////////////////////////////////////////////////
	bool Combo(const char* label, std::string& val, std::vector<std::string> const& options);

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	bool Combo(const char* label, std::string& val, std::unordered_map<std::string, T> const& members)
	{
		std::vector<const char*> options(members.size());
		std::transform(members.begin(), members.end(), options.begin(), [](auto const& v) { return v.first.c_str(); });
		int valId = std::distance(options.begin(), std::find(options.begin(), options.end(), val));
		bool changed = ImGui::Combo(label, &valId, options.data(), options.size());
		if (valId < options.size()) val = options[valId];
		return changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	bool Combo(const char* label, std::string& val, std::map<std::string, T> const& members)
	{
		std::vector<const char*> options(members.size());
		std::transform(members.begin(), members.end(), options.begin(), [](auto const& v) { return v.first.c_str(); });
		int valId = std::distance(options.begin(), std::find(options.begin(), options.end(), val));
		bool changed = ImGui::Combo(label, &valId, options.data(), options.size());
		if (valId < options.size()) val = options[valId];
		return changed;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename A, size_t N>
	bool Combo(const char* label, A* val, std::array<A, N> const& members)
	{
		return Combo(label, (int*) val, std::vector<std::string>(members.begin(), members.end()));
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename EnumType, typename UnderlyingType, size_t size>
	bool Combo(const char* label, EnumType* val, MetaEnum<EnumType, UnderlyingType, size> const& meta)
	{
		return Combo(label, (int*) val, std::meta_enum_names(meta));
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename E>
	bool Combo(const char* label, typename E::EnumType* val, E const& meta, std::vector<typename E::EnumType> const& enums)
	{
		*val = std::distance(enums.begin(), std::find(enums.begin(), enums.end(), *val));
		bool result = Combo(label, (int*)val, meta_enum_names(meta, enums));
		*val = enums[*val];
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool RadioButton(const char* label, int* val, std::vector<const char*> const& options, bool sameLine = true);

	////////////////////////////////////////////////////////////////////////////////
	bool RadioButton(const char* label, int* val, std::vector<std::string> const& options, bool sameLine = true);

	////////////////////////////////////////////////////////////////////////////////
	template<typename A, size_t N>
	bool RadioButton(const char* label, int* val, std::array<A, N> const& members, bool sameLine = true)
	{
		return RadioButton(label, val, std::vector<std::string>(members.begin(), members.end()), sameLine);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename E>
	bool RadioButton(const char* label, int* val, E const& meta, bool sameLine = true)
	{
		return RadioButton(label, val, meta_enum_names(meta), sameLine);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename E>
	bool RadioButton(const char* label, int* val, E const& meta, std::vector<typename E::EnumType> const& enums, bool sameLine = true)
	{
		*val = std::distance(enums.begin(), std::find(enums.begin(), enums.end(), *val));
		bool result = RadioButton(label, val, meta_enum_names(meta, enums), sameLine);
		*val = enums[*val];
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool SelectableWithIcon(const char* label, bool item_selected, ImTextureID user_texture_id, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	////////////////////////////////////////////////////////////////////////////////
	void ImageSquare(ImTextureID user_texture_id, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	////////////////////////////////////////////////////////////////////////////////
	bool ButtonEx(const char* label, const char* referenceLabel, ImGuiButtonFlags flags = 0);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCustomClassic(ImGuiStyle* dst = NULL, float dpiScale = 1.0f, bool lowres = false);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCustomDark(ImGuiStyle* dst = NULL, float dpiScale = 1.0f, bool lowres = false);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCustomLight(ImGuiStyle* dst = NULL, float dpiScale = 1.0f, bool lowres = false);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsIntelliJDracula(ImGuiStyle* dst = NULL, float dpiScale = 1.0f);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsMaya(ImGuiStyle* dst = NULL, float dpiScale = 1.0f);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsMicrosoftLight(ImGuiStyle* dst = NULL, float dpiScale = 1.0f);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsUnreal(ImGuiStyle* dst = NULL, float dpiScale = 1.0f, bool lowres = false);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsExtasy(ImGuiStyle* dst = NULL, float dpiScale = 1.0f);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsPhotoshop(ImGuiStyle* dst = NULL, float dpiScale = 1.0f);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsCorporateGrey(ImGuiStyle* dst = NULL, float dpiScale = 1.0f, bool is3D = true);

	////////////////////////////////////////////////////////////////////////////////
	void StyleColorsRudaDark(ImGuiStyle* dst = NULL, float dpiScale = 1.0f);

	////////////////////////////////////////////////////////////////////////////////
	int GetNumStyles();

	////////////////////////////////////////////////////////////////////////////////
	const char* StyleColorsById(int id, ImGuiStyle* dst, float dpiScale = 1.0f);
}