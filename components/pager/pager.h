#ifndef PAGER_PAGER_H_
#define PAGER_PAGER_H_

/****************************************************************
 * Includes
 ****************************************************************/
// cstdlib includes
#include <stdint.h>
#include <stdbool.h>

// TFT includes
#include "tft/tft.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define PAGE_NAME_SIZE	(16)

/****************************************************************
 * Typedefs, structs, enums
 ****************************************************************/
typedef struct page
{
	struct page *	nextPage;
	char 			name[PAGE_NAME_SIZE];
	bool			(* initPage)();
	bool			(* deInitPage)();
	void 			(* renderPage)(bool force);
	void			(* action)();
	color_t			header_fg;
	color_t			header_bg;
	uint32_t		update_period;
} page_t;

/****************************************************************
 * Function declarations
 ****************************************************************/
bool Pager_Init();

void Pager_Tick();

void Pager_Action();

bool Pager_AddPage(page_t * page);

bool Pager_NextPage();

#endif /* PAGER_PAGER_H_ */
