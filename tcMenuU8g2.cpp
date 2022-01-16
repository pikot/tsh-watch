/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

/**
 * @file tcMenuU8g2.h
 * 
 * U8g2 renderer that renders menus onto this type of display. This file is a plugin file and should not
 * be directly edited, it will be replaced each time the project is built. If you want to edit this file in place,
 * make sure to rename it first.
 * 
 * LIBRARY REQUIREMENT
 * This library requires the u8g2 library available for download from your IDE library manager.
 */

#include <U8g2lib.h>
#include "tcMenuU8g2.h"
#include "utils.h"
#include "version.h"

const char MENU_BACK_TEXT[] PROGMEM = "[..]";

void U8g2MenuRenderer::setGraphicsDevice(U8G2* u8g2, U8g2GfxMenuConfig *gfxConfig) {

	  this->u8g2 = u8g2;
	  this->gfxConfig = gfxConfig;

	  if (gfxConfig->editIcon == NULL || gfxConfig->activeIcon == NULL) {
		    gfxConfig->editIcon = defEditingIcon;
		    gfxConfig->activeIcon = defActiveIcon;
		    gfxConfig->editIconWidth = 16;
		    gfxConfig->editIconHeight = 12;
	  }

    // font cannot be NULL on this board, we default if it is.
    if (gfxConfig->itemFont == NULL) gfxConfig->itemFont = u8g2_font_6x10_tf;
    if (gfxConfig->titleFont == NULL) gfxConfig->titleFont = u8g2_font_6x10_tf;
}

U8g2MenuRenderer::~U8g2MenuRenderer() {
}

void U8g2MenuRenderer::renderTitleArea() {
	  if (menuMgr.getCurrentMenu() == menuMgr.getRoot()) {
		    safeProgCpy(buffer, applicationInfo.name, bufferSize);
	  }
	  else {
		    menuMgr.getCurrentMenu()->copyNameToBuffer(buffer, bufferSize);
	  }

    serdebugF3("Render title, fontMag: ", buffer, gfxConfig->titleFontMagnification);

    u8g2->setFont(gfxConfig->titleFont);
    u8g2->setFontRefHeightExtendedText();
    u8g2->setFontDirection(0);

	  int extentY = u8g2->getMaxCharHeight();
	  titleHeight = extentY + gfxConfig->titlePadding.top + gfxConfig->titlePadding.bottom;
    int fontYStart = titleHeight - (gfxConfig->titlePadding.bottom);

    serdebugF3("titleHeight, fontYStart: ",  titleHeight, fontYStart);

	  u8g2->setColorIndex(gfxConfig->bgTitleColor);
	  u8g2->drawBox(0, 0, u8g2->getDisplayWidth(), titleHeight);
	  u8g2->setColorIndex(gfxConfig->fgTitleColor);
	  u8g2->setCursor(gfxConfig->titlePadding.left, fontYStart);
	  u8g2->print(buffer);
	  titleHeight += gfxConfig->titleBottomMargin;
}

void U8g2MenuRenderer::drawBitmap(int x, int y, int w, int h, const unsigned char *bmp) {
#if defined(__AVR__) || defined(ESP8266)
    u8g2->drawXBMP(x, y, w, h, bmp);
#else
    u8g2->drawXBM(x, y, w, h, bmp);
#endif
}

bool U8g2MenuRenderer::renderWidgets(bool forceDraw) {
	  TitleWidget* widget = firstWidget;
	  int xPos = u8g2->getDisplayWidth() - gfxConfig->widgetPadding.right;
    bool redrawNeeded = forceDraw;
	  while(widget) {
		    xPos -= widget->getWidth();

		    if(widget->isChanged() || forceDraw) {
            redrawNeeded = true;

            serdebugF3("Drawing widget pos,icon: ", xPos, widget->getCurrentState());
            u8g2->setColorIndex(gfxConfig->widgetColor);
			      drawBitmap(xPos, gfxConfig->widgetPadding.top, widget->getWidth(), widget->getHeight(), widget->getCurrentIcon());
		    }

		    widget = widget->getNext();
		    xPos -= gfxConfig->widgetPadding.left;
	  }
    return redrawNeeded;
}

void U8g2MenuRenderer::renderListMenu(int titleHeight) {
    ListRuntimeMenuItem* runList = reinterpret_cast<ListRuntimeMenuItem*>(menuMgr.getCurrentMenu());

    uint8_t maxY = uint8_t((u8g2->getDisplayHeight() - titleHeight) / itemHeight);
	  maxY = min(maxY, runList->getNumberOfParts());
	  uint8_t currentActive = runList->getActiveIndex();

	  uint8_t offset = 0;
	  if (currentActive >= maxY) {
		    offset = (currentActive+1) - maxY;
	  }

    int yPos = titleHeight;
	  for (int i = 0; i < maxY; i++) {
		    uint8_t current = offset + i;
		    RuntimeMenuItem* toDraw = (current==0) ? runList->asBackMenu() : runList->getChildItem(current - 1);
		    renderMenuItem(yPos, itemHeight, toDraw);
        yPos += itemHeight;
	  }

	  // reset the list item to a normal list again.
	  runList->asParent();
}

void U8g2MenuRenderer::render() {
 	  if (u8g2 == NULL) return;

	  uint8_t locRedrawMode = redrawMode;
	  redrawMode = MENUDRAW_NO_CHANGE;
    bool requiresUpdate = false;

	  countdownToDefaulting();

	  if (locRedrawMode == MENUDRAW_COMPLETE_REDRAW) {
	    // pre-populate the font.
        u8g2->setFont(gfxConfig->itemFont);
        u8g2->setFontPosBottom();
        itemHeight = u8g2->getMaxCharHeight();

        // clear screen first in complete draw mode
        u8g2->setColorIndex(gfxConfig->bgItemColor);
        u8g2->drawBox(0,0,u8g2->getDisplayWidth(), u8g2->getDisplayHeight());

       	itemHeight = itemHeight + gfxConfig->itemPadding.top + gfxConfig->itemPadding.bottom;
        serdebugF2("Redraw all, new item height ", itemHeight);

		    renderTitleArea();
        taskManager.yieldForMicros(0);

        renderWidgets(true);
        taskManager.yieldForMicros(0);
        requiresUpdate = true;
	  }
	  else {
        requiresUpdate = renderWidgets(false);
	  }

	  u8g2->setFont(gfxConfig->itemFont);
	  int maxItemsY = ((u8g2->getDisplayHeight()-titleHeight) / itemHeight);

    if(menuMgr.getCurrentMenu()->getMenuType() == MENUTYPE_RUNTIME_LIST) {
        if(menuMgr.getCurrentMenu()->isChanged() || locRedrawMode != MENUDRAW_NO_CHANGE) {
            requiresUpdate = true;
            renderListMenu(titleHeight);
        }
    }
    else {
        MenuItem* item = menuMgr.getCurrentMenu();
        // first we find the first currently active item in our single linked list
        uint8_t currActiveOffs = offsetOfCurrentActive(item);
        if (currActiveOffs >= maxItemsY) {
            uint8_t toOffsetBy = (currActiveOffs - maxItemsY) + 1;

            if(lastOffset != toOffsetBy) locRedrawMode = MENUDRAW_COMPLETE_REDRAW;
            lastOffset = toOffsetBy;

			      while (item != NULL && toOffsetBy) {
                if (item->isVisible()) toOffsetBy = toOffsetBy - 1;
				        item = item->getNext();
			      }
        }
        else {
            if (lastOffset != 0xff) locRedrawMode = MENUDRAW_COMPLETE_REDRAW;
            lastOffset = 0xff;
        }

        //serdebugF3("Offset chosen, on display", lastOffset, maxItemsY);

        // and then we start drawing items until we run out of screen or items
        int ypos = titleHeight;
        while (item && (ypos + itemHeight) <= u8g2->getDisplayHeight() ) {
            if(item->isVisible())
            {
                if (locRedrawMode != MENUDRAW_NO_CHANGE || item->isChanged()) {
                    requiresUpdate = true;

                    taskManager.yieldForMicros(0);

                    renderMenuItem(ypos, itemHeight, item);
                }
                ypos += itemHeight;
            }
            item = item->getNext();                                                                                                                                                                                                                                  
        }
    }

    if (requiresUpdate) u8g2->sendBuffer();  // !!!!!!!!!!!!!!!!!!!!!
}

void U8g2MenuRenderer::renderMenuItem(int yPos, int menuHeight, MenuItem* item) {
	  int icoWid = gfxConfig->editIconWidth;
	  int icoHei = gfxConfig->editIconHeight;

	  item->setChanged(false); // we are drawing the item so it's no longer changed.

    int imgMiddleY = yPos + ((menuHeight - icoHei) / 2);

	  if (item->isEditing()) {
        u8g2->setColorIndex(gfxConfig->bgSelectColor);
		    u8g2->drawBox(0, yPos, u8g2->getDisplayWidth(), menuHeight);
		    u8g2->setColorIndex(gfxConfig->fgSelectColor);
		    drawBitmap(gfxConfig->itemPadding.left, imgMiddleY, icoWid, icoHei, gfxConfig->editIcon);
        serdebugF("Item Editing");
	  }
	  else if (item->isActive()) {
		    u8g2->setColorIndex(gfxConfig->bgSelectColor);
		    u8g2->drawBox(0, yPos, u8g2->getDisplayWidth(), menuHeight);
		    u8g2->setColorIndex(gfxConfig->fgSelectColor);
		    drawBitmap(gfxConfig->itemPadding.left, imgMiddleY, icoWid, icoHei, gfxConfig->activeIcon);
        serdebugF("Item Active");
	  }
	  else {
        u8g2->setColorIndex(gfxConfig->bgItemColor);
		    u8g2->drawBox(0, yPos, u8g2->getDisplayWidth(), menuHeight);
		    u8g2->setColorIndex(gfxConfig->fgItemColor);
        serdebugF("Item Normal");
	  }

    taskManager.yieldForMicros(0);

	  int textPos = gfxConfig->itemPadding.left + icoWid + gfxConfig->itemPadding.left;
	
	  int drawingPositionY = yPos + gfxConfig->itemPadding.top;
	  if (gfxConfig->itemFont) {
		    drawingPositionY = yPos + (menuHeight - (gfxConfig->itemPadding.bottom));
	  }
	  u8g2->setCursor(textPos, drawingPositionY);
	  item->copyNameToBuffer(buffer, bufferSize);

    serdebugF4("Printing menu item (name, ypos, drawingPositionY)", buffer, yPos, drawingPositionY);

	  u8g2->print(buffer);

    if (isItemActionable(item)) {
        int rightOffset = u8g2->getDisplayWidth() - (gfxConfig->itemPadding.right + icoWid);
		    u8g2->setColorIndex(gfxConfig->fgSelectColor);
		    drawBitmap(rightOffset, imgMiddleY, icoWid, icoHei, gfxConfig->activeIcon);
        buffer[0] = 0;
    } 
    else if (item->getMenuType() == MENUTYPE_BACK_VALUE) {
        safeProgCpy(buffer, MENU_BACK_TEXT, bufferSize);
    }
    else {
	      menuValueToText(item, JUSTIFY_TEXT_LEFT);
    }

	  int16_t right = u8g2->getDisplayWidth() - (u8g2->getStrWidth(buffer) + gfxConfig->itemPadding.right);
	  u8g2->setCursor(right, drawingPositionY);
 	  u8g2->print(buffer);
    serdebugF2("Value ", buffer);

    taskManager.yieldForMicros(0);
}

/**
 * Provides a basic graphics configuration suitable for low / medium resolution displays
 * @param config usually a global variable holding the graphics configuration.
 */
void prepareBasicU8x8Config(U8g2GfxMenuConfig& config) {
	  makePadding(config.titlePadding, 1, 1, 1, 1);
	  makePadding(config.itemPadding, 1, 1, 1, 1);
	  makePadding(config.widgetPadding, 2, 2, 0, 2);

	  config.bgTitleColor = WHITE;
	  config.fgTitleColor = BLACK;
	  config.titleBottomMargin = 1;
	  config.widgetColor = BLACK;
	  config.titleFontMagnification = 1;

	  config.bgItemColor = BLACK;
	  config.fgItemColor = WHITE;
	  config.bgSelectColor = BLACK;
	  config.fgSelectColor = WHITE;
	  config.itemFontMagnification = 1;

    config.editIcon = loResEditingIcon;
    config.activeIcon = loResActiveIcon;


#ifdef IS_V1_4
    config.editIconHeight = 11;
    config.editIconWidth = 13;
    config.titleFont = u8g2_font_crox5h_tr;
    config.itemFont = u8g2_font_crox4h_tr;
    printf_w("IS_V1_4 \n");
#else
    config.editIconHeight = 6;
    config.editIconWidth = 8;
    config.titleFont = u8g2_font_6x12_tf;
    config.itemFont = u8g2_font_6x10_tf;
    printf_w("IS_old \n");
#endif
    
}

BaseDialog* U8g2MenuRenderer::getDialog() {
    if(dialog == NULL) {
        dialog = new U8g2Dialog();
    }
    return dialog;
}


#define ROW_MAX 4

void U8g2Dialog::drawText(int _x, int _y, char *line_buf) {
    U8g2MenuRenderer* adaRenderer = reinterpret_cast<U8g2MenuRenderer*>(MenuRenderer::getInstance());
    U8G2* graphics = adaRenderer->getGraphics();
    graphics->setFont(u8g_font_5x7);

    uint8_t rows; // calc on each loop bad idea
    uint8_t fontLineSpacing = 9;
    rows = graphics->getHeight() / fontLineSpacing;
    
    if ( rows > ROW_MAX )
        rows = ROW_MAX; 
    printf_w("drawText: x %d, y %d, rows %d, line_buf %s\n", _x, _y, rows, line_buf);

    uint8_t i, y;
    // graphic commands to redraw the complete screen are placed here    
    y = _y;       // reference is the top left -1 position of the string
    y--;           // correct the -1 position of the drawStr 
    
    int line_start = 0, line_len = 0, lines_cnt = 0;
    bool  new_line = true;
    for ( i = 0; (0 != line_buf[i]) && ( lines_cnt < rows) ; i++ )
    {
        printf_w("drawText: i %d, lines_cnt %d, %c\n", i, lines_cnt,  line_buf[i]);
        if ( true == new_line) {
              line_start = i;
              new_line = false;
        }
        if ( '\n' == line_buf[i] || '\0' == line_buf[i] ) {
              line_buf[i] = '\0'; 
              graphics->drawStr( _x, y, (char *) ( &line_buf[line_start] )  );
              printf_w("drawText: x %d, y %d, line_start %d, i %d,  %s\n", _x, y, line_start, i, (char *) ( &line_buf[line_start] ));
              y += fontLineSpacing;
              new_line = true;
              line_len   = 0;
              lines_cnt  +=1;
        }
        else {
              line_len +=1;
        }
    }
    if ( line_len > 0 ) {
              graphics->drawStr( _x, y, (char *) ( &line_buf[line_start] )  );
              printf_w("drawText: x %d, y %d, line_start %d, i %d,  %s\n", _x, y, line_start, i, (char *) ( &line_buf[line_start] ));
    }
    
    
}

void U8g2Dialog::internalRender(int currentValue) {
    U8g2MenuRenderer* adaRenderer = reinterpret_cast<U8g2MenuRenderer*>(MenuRenderer::getInstance());
    U8g2GfxMenuConfig* gfxConfig = adaRenderer->getGfxConfig();
    U8G2* graphics = adaRenderer->getGraphics();

    if(needsDrawing == MENUDRAW_COMPLETE_REDRAW) {
        // clear screen first in complete draw mode
        graphics->setColorIndex(gfxConfig->bgItemColor);
        graphics->drawBox(0,0,graphics->getDisplayWidth(), graphics->getDisplayHeight());
    }

    graphics->setFont(gfxConfig->itemFont);

    char data[20];
    safeProgCpy(data, headerPgm, sizeof(data));

	  int fontYStart = gfxConfig->itemPadding.top;
  	int extentY = graphics->getMaxCharHeight();
	  int dlgNextDraw = extentY + gfxConfig->titlePadding.top + gfxConfig->titlePadding.bottom;
	  if (gfxConfig->itemFont) {
	 	    fontYStart = dlgNextDraw - (gfxConfig->titlePadding.bottom);
	  }

    graphics->setColorIndex(gfxConfig->bgTitleColor);
	  graphics->drawBox(0, 0, graphics->getDisplayWidth(), dlgNextDraw);
	  graphics->setColorIndex(gfxConfig->fgTitleColor);
	  graphics->setCursor(gfxConfig->titlePadding.left, fontYStart);
	  graphics->print(data);

	  dlgNextDraw += gfxConfig->titleBottomMargin;

    int startingPosition = dlgNextDraw;
    fontYStart = dlgNextDraw + gfxConfig->itemPadding.top;
	  dlgNextDraw = dlgNextDraw + extentY + gfxConfig->titlePadding.top + gfxConfig->titlePadding.bottom;
	  if (gfxConfig->itemFont) {
	 	    fontYStart = dlgNextDraw - (gfxConfig->titlePadding.bottom);
	  }
    graphics->setColorIndex(gfxConfig->bgItemColor);
    graphics->drawBox(0, startingPosition, graphics->getDisplayWidth(), dlgNextDraw);
	  graphics->setColorIndex(gfxConfig->fgItemColor);
//	  graphics->setCursor(gfxConfig->titlePadding.left, fontYStart);
	 // graphics->print(MenuRenderer::getInstance()->getBuffer());
    drawText(gfxConfig->titlePadding.left, fontYStart, MenuRenderer::getInstance()->getBuffer());

    bool active;
    if(button1 != BTNTYPE_NONE) {
        active = copyButtonText(data, 0, currentValue);
        drawButton(graphics, gfxConfig, data, 0, active);
    }
    if(button2 != BTNTYPE_NONE) {
        active = copyButtonText(data, 1, currentValue);
        drawButton(graphics, gfxConfig, data, 1, active);
    }
    graphics->sendBuffer();
}

void U8g2Dialog::drawButton(U8G2* gfx, U8g2GfxMenuConfig* config, const char* title, uint8_t num, bool active) {
	  int extentY = gfx->getMaxCharHeight();
    int itemHeight = ( extentY + config->itemPadding.top + config->itemPadding.bottom);
    int start = gfx->getDisplayHeight() - itemHeight;
    int fontYStart = start + config->itemPadding.top;
	  if (config->itemFont) {
        fontYStart += extentY;
	  }
    int buttonWidth = gfx->getDisplayWidth() / 2;
    int xOffset = (num == 0) ? 0 : buttonWidth;
    gfx->setColorIndex(active ? config->bgSelectColor : config->bgItemColor);
    gfx->drawBox(xOffset, start, buttonWidth, itemHeight);
	  gfx->setColorIndex(active ? config->fgSelectColor : config->fgItemColor);
    gfx->setCursor(xOffset + ((buttonWidth - gfx->getStrWidth(title)) / 2), fontYStart);
    gfx->print(title);
}
