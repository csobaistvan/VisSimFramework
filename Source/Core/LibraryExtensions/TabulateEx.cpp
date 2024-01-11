#include "PCH.h"
#include "TabulateEx.h"

////////////////////////////////////////////////////////////////////////////////
namespace tabulate
{
	////////////////////////////////////////////////////////////////////////////////
	Row& headerTopRow(Table& table)
	{
		return table.row(0);
	}

	////////////////////////////////////////////////////////////////////////////////
	Row& headerBotRow(Table& table)
	{
		return table.row(1);
	}

	////////////////////////////////////////////////////////////////////////////////
	Row& lastRow(Table& table)
	{
		Table::RowIterator resultIt = table.begin();
		for (auto it = table.begin(); it != table.end(); ++it)
			resultIt = it;
		return *resultIt;
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace formats
	{
		////////////////////////////////////////////////////////////////////////////////
		void treelike(Table& table)
		{
			// Remove all top and bottom lines and set corners and horizontal borders to spaces
			table.format().locale("C").
				hide_border().
				show_border_left().border_left(" ").
				show_border_right().border_right(" ").
				corner(" ");

			// Apply the correct separators around the header
			headerTopRow(table).format().
				show_border_bottom().border_bottom("-").corner(" ");
			headerBotRow(table).format().
				show_border_top().border_top("-").corner(" ");
		}

		////////////////////////////////////////////////////////////////////////////////
		void psql(Table& table)
		{
			// Remove all top and bottom lines, and set the corners to pipes
			table.format().locale("C").hide_border_bottom().hide_border_top().corner("|");
			
			// Apply the correct separators around the header
			headerTopRow(table).format().show_border_bottom().border_bottom("-").show_border_top().border_top("-").corner("+");
			headerBotRow(table).format().show_border_bottom().border_bottom("-").show_border_top().border_top("-").corner("+");

			// Apply the correct separators to the last row
			lastRow(table).format().show_border_bottom().border_bottom("-").corner_bottom_left("+").corner_bottom_right("+");
		}
	}
}