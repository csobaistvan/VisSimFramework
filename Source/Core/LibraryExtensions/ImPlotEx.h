#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Core/Constants.h"

////////////////////////////////////////////////////////////////////////////////
/// IMPLOT EXTENSIONS
////////////////////////////////////////////////////////////////////////////////
namespace ImPlot
{
	////////////////////////////////////////////////////////////////////////////////
	struct MouseHitLineResult
	{
		bool m_inside = false;
		int m_index = 0;
		float m_frac = 0.0f;
		ImPlotPoint m_prev{ 0.0f, 0.0f };
		ImPlotPoint m_next{ 0.0f, 0.0f };
		ImPlotPoint m_hit{ 0.0f, 0.0f };
	};

	////////////////////////////////////////////////////////////////////////////////
	MouseHitLineResult FindMouseHitLineG(ImPlotPoint(*getter)(void* usr, int i), void* userData, int count, ImPlotPoint const& mouse);

	////////////////////////////////////////////////////////////////////////////////
	// TODO: find a way to extract most of these parameters automatically
	void HighlightMouseHitLineG(MouseHitLineResult const& mouseHit, ImU32 markerColor, float markerRadius = 4.0f, ImU32 barColor = IM_COL32(128, 128, 128, 64));

	////////////////////////////////////////////////////////////////////////////////
	struct MouseHitBarResult
	{
		bool m_inside = false;
		int m_index = 0;
		ImPlotPoint m_prev{ 0.0f, 0.0f };
		ImPlotPoint m_next{ 0.0f, 0.0f };
		ImPlotPoint m_hit{ 0.0f, 0.0f };
	};

	////////////////////////////////////////////////////////////////////////////////
	MouseHitBarResult FindMouseHitBarG(ImPlotPoint(*getter)(void* usr, int i), void* userData, int count, float barWidth, ImPlotPoint const& mouse);

	////////////////////////////////////////////////////////////////////////////////
	// TODO: find a way to extract most of these parameters automatically
	// TODO: more sophisticated highlighting
	void HighlightMouseHitBarG(MouseHitBarResult const& mouseHit, ImU32 barColor = IM_COL32(128, 128, 128, 64));
}