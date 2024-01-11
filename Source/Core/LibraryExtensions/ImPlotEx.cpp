#include "PCH.h"
#include "ImPlotEx.h"

#include "StdEx.h"
#include "Core/Preprocessor.h"
#include "Core/Debug.h"

namespace ImPlot
{
	////////////////////////////////////////////////////////////////////////////////
	MouseHitLineResult FindMouseHitLineG(ImPlotPoint(*getter)(void* usr, int i), void* userData, int count, ImPlotPoint const& mouse)
	{
		// Result of the query
		MouseHitLineResult result;

		// Plot x limits
		const ImPlotPoint p0 = getter(userData, 0);
		const ImPlotPoint pn = getter(userData, count - 1);
		const float xMin = std::min(p0.x, pn.x);
		const float xMax = std::max(p0.x, pn.x);

		// Early out if we are outside the bounds
		if (mouse.x < xMin || mouse.x > xMax)
			return result;

		// Search for the corresponding index
		int idx = -1;
		ImPlotPoint prev = p0;
		ImPlotPoint next = p0;
		for (int i = 1; i < count; ++i)
		{
			next = getter(userData, i);
			if (mouse.x >= std::min(prev.x, next.x) && mouse.x <= std::max(prev.x, next.x))
			{
				idx = i;
				break;
			}
			prev = next;
		}

		// Fill in the result structure
		result.m_inside = true;
		result.m_index = idx;
		result.m_prev = prev;
		result.m_next = next;
		result.m_frac = (mouse.x - prev.x) / (next.x - prev.x);
		result.m_hit = ImPlotPoint(mouse.x, (1.0f - result.m_frac) * prev.y + result.m_frac * next.y);

		// Return the result
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	void HighlightMouseHitLineG(MouseHitLineResult const& mouseHit, ImU32 markerColor, float markerRadius, ImU32 barColor)
	{
		// Tooltip vertical bar and highlight markers
		ImDrawList* draw_list = GetPlotDrawList();
		const ImVec2 plotSize = GetPlotSize();
		const float tool_l = PlotToPixels(mouseHit.m_hit.x, 0.0f).x - plotSize.x * 1e-3f;
		const float tool_r = PlotToPixels(mouseHit.m_hit.x, 0.0f).x + plotSize.x * 1e-3f;
		const float tool_t = GetPlotPos().y;
		const float tool_b = tool_t + plotSize.y;
		PushPlotClipRect();
		draw_list->AddRectFilled(ImVec2(tool_l, tool_t), ImVec2(tool_r, tool_b), barColor);
		draw_list->AddCircleFilled(PlotToPixels(ImVec2(mouseHit.m_hit.x, mouseHit.m_hit.y)), markerRadius, markerColor);
		PopPlotClipRect();
	}

	////////////////////////////////////////////////////////////////////////////////
	MouseHitBarResult FindMouseHitBarG(ImPlotPoint(*getter)(void* usr, int i), void* userData, int count, float barWidth, ImPlotPoint const& mouse)
	{
		// Result of the query
		MouseHitBarResult result;

		// Plot x limits
		const ImPlotPoint p0 = getter(userData, 0);
		const ImPlotPoint pn = getter(userData, count - 1);
		const float xMin = std::min(p0.x, pn.x);
		const float xMax = std::max(p0.x, pn.x);

		// Half bar size
		const float halfWidth = barWidth / 2.0f;

		// Early out if we are outside the bounds
		if (mouse.x < xMin - halfWidth || mouse.x > xMax + halfWidth)
			return result;

		// Search for the corresponding index
		int idx = -1;
		ImPlotPoint bar;
		for (int i = 0; i < count; ++i)
		{
			bar = getter(userData, i);
			if (mouse.x >= bar.x - halfWidth && mouse.x <= bar.x + halfWidth)
			{
				idx = i;
				break;
			}
		}

		// Early out if we did not hit any bar
		if (idx == -1)
			return result;

		// Fill in the result structure
		result.m_inside = true;
		result.m_index = idx;
		result.m_prev = ImPlotPoint(bar.x - halfWidth, bar.y);
		result.m_next = ImPlotPoint(bar.x + halfWidth, bar.y);
		result.m_hit = bar;

		// Return the result
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	void HighlightMouseHitBarG(MouseHitBarResult const& mouseHit, ImU32 barColor)
	{
		// Tooltip vertical bar and highlight markers
		ImDrawList* draw_list = GetPlotDrawList();
		const ImVec2 plotSize = GetPlotSize();
		const float tool_l = PlotToPixels(mouseHit.m_hit.x, 0.0f).x - plotSize.x * 1e-3f;
		const float tool_r = PlotToPixels(mouseHit.m_hit.x, 0.0f).x + plotSize.x * 1e-3f;
		const float tool_t = GetPlotPos().y;
		const float tool_b = tool_t + plotSize.y;
		PushPlotClipRect();
		draw_list->AddRectFilled(ImVec2(tool_l, tool_t), ImVec2(tool_r, tool_b), barColor);
		PopPlotClipRect();
	}
}