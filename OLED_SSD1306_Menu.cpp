#include "OLED_SSD1306_Menu.h"

OLED_SSD1306_Menu_Item::OLED_SSD1306_Menu_Item(float (*update)(float, Value*), float value, char* label)
{
  _update.f = update;
  _type = Float;
  _value.f = value;
  _label = label;
}

OLED_SSD1306_Menu_Item::OLED_SSD1306_Menu_Item(int (*update)(int, Value*), int value, char* label)
{
  _update.i = update;
  _type = Int;
  _value.i = value;
  _label = label;
}

OLED_SSD1306_Menu_Item::OLED_SSD1306_Menu_Item(char* (*update)(char*, Value*), char* value, char* label)
{
  _update.c = update;
  _type = Char;
  _value.c = value;
  _label = label;
}

OLED_SSD1306_Menu_Item::OLED_SSD1306_Menu_Item(bool (*update)(bool, Value*), bool value, char* label)
{
  _update.b = update;
  _type = Bool;
  _value.b = value;
  _label = label;
}

OLED_SSD1306_Menu_Item::OLED_SSD1306_Menu_Item(void (*update)(Value*), char* value)
{
  _update.v = update;
  _type = Button;
  _value.c = value;
}

void OLED_SSD1306_Menu_Item::setCapture(Value *capture) {
    _capture = capture;
}

OLED_SSD1306_Menu_Item* OLED_SSD1306_Menu_Item::clone() const {
  return new OLED_SSD1306_Menu_Item(*this); // invoke the copy constructor
}
void OLED_SSD1306_Menu_Item::draw(Adafruit_SSD1306 *display, bool selected, bool hovered, int y, int width)
{
    if(hovered) {
        display->drawPixel(2, y + 4, WHITE);
    }

    float currentTime = millis();

    // blinks an dot next to the item if it is selected to indicate that it is selected.
    if(selected && hovered && currentTime >= lastDelay + selectedBlinkDelay) {
        display->drawPixel(2, y + 4, BLACK);
        if(currentTime > lastDelay + selectedBlinkDelay + selectedBlinkDuration)
            lastDelay = currentTime;
    } 

    display->setCursor(5, y);

    if(_type != Button) {
        display->print(_label);
    }

    switch (_type)
    {
        case Float:
            char buffer1[20]; // The buffer to store the result
            sprintf(buffer1, "%4.2f", _value.f);

            OLED_SSD1306_Menu::drawRightAlignedString(display, buffer1, width, y);
            break;
        case Int:
            char buffer2[20]; // The buffer to store the result
            sprintf(buffer2, "%4d", _value.i);

            OLED_SSD1306_Menu::drawRightAlignedString(display, buffer2, width, y);
            break;
        case Char:
            OLED_SSD1306_Menu::drawRightAlignedString(display, _value.c, width, y);
            break;
        case Bool:
            OLED_SSD1306_Menu::drawRightAlignedString(display, _value.b ? "true" : "false", width, y);
            break;
        case Button:
            display->print(_value.c);
            break;
        default:
            display->print(F("undefined"));
            break;
    }
}

Value OLED_SSD1306_Menu_Item::getValue() 
{
    return _value;
}

void OLED_SSD1306_Menu_Item::update(bool selected)
{
    switch (_type)
    {
        case Float:
            _value.f = _update.f(_value.f, _capture);
            break;
        case Int:
            _value.i = _update.i(_value.i, _capture);
            break;
        case Bool:
            _value.b = _update.b(_value.b, _capture);
            break;
        case Char:
            _value.c = _update.c(_value.c, _capture);
            break;
        case Button:
            _update.v(_capture);
            break;
    }
}

void OLED_SSD1306_Menu::drawRightAlignedString(Adafruit_SSD1306 *display, const char *buf, uint8_t width, uint8_t y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds(buf, width, y, &x1, &y1, &w, &h); //calc width of new string
    display->setCursor(width - w, y);
    display->print(buf);
}

void OLED_SSD1306_Menu::drawCentreString(Adafruit_SSD1306 *display, const char *buf, uint8_t x, uint8_t y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    display->setCursor((x - w) / 2, y);
    display->print(buf);
}


void OLED_SSD1306_Menu::addItem(OLED_SSD1306_Menu_Item *item)
{
    _items[_itemCount] = item;
    _itemCount++;

}

uint8_t OLED_SSD1306_Menu::getItemCount()
{
    return _itemCount;
}

int OLED_SSD1306_Menu::calculateInitialOffset() 
{
    uint8_t nextOffset = 10;

    for (uint8_t i = 0; i < _itemCount; i++)
    {
        uint8_t y = i * 10 + nextOffset;
        uint8_t height = 10;
        nextOffset = height;
        if(_hovered != i)
            continue;
        if(y + height > _height) {
            int nextOffset2 = 0;
            for(uint8_t j = i - 1; j > 0; j--) 
            {
                int height2 = 10;
                nextOffset2 = nextOffset2 - height2;

                if(y + height + nextOffset2 <= _height) 
                    return nextOffset2 + 10;
            }
        }

        return 10;
    }

    return 10;
}

void OLED_SSD1306_Menu::_draw()
{
    _display->clearDisplay();
    _display->setCursor(0, 0);
    _display->setTextColor(WHITE);
    _display->print(_hovered);

    uint8_t titleHeight = 10;

    drawCentreString(_display, label, _width, 0);

    int initialOffset = calculateInitialOffset(); //the initial offset for items.

    int offset = initialOffset;

    for (uint8_t i = 0; i < _itemCount; i++)
    {
        OLED_SSD1306_Menu_Item* item = _items[i];

        uint8_t height = 10;

        if(offset >= titleHeight)
            _display->drawFastHLine(0, offset - 2, _width, WHITE);

        if(offset + height <= _height && offset >= titleHeight)
            item->draw(_display, _hovered == i && _selected, _hovered == i, offset, _width);
        else if(offset > _height)
            break;

        offset += height;
    }

    _display->display();
}

void OLED_SSD1306_Menu::clear()
{
    // Assuming _items is a member of your class and items is the size of the array
    for(int i = 0; i < _itemCount; i++) {
        delete _items[i];  // This deletes the objects pointed to by the array elements
        _items[i] = nullptr;
    }
    _itemCount = 0;

}
void OLED_SSD1306_Menu::update()
{
    if(_selected) {
        OLED_SSD1306_Menu_Item* selectedItem = _items[_hovered];
        selectedItem->update(true); //updates the selected item
    }

    _draw(); //redraws the menu
}

void OLED_SSD1306_Menu::swap(int index1, int index2)
{
    OLED_SSD1306_Menu_Item *temp = _items[index1]; // store the value of arr[index1] in temp
    _items[index1] = _items[index2]; // assign the value of arr[index2] to arr[index1]
    _items[index2] = temp;
}

void OLED_SSD1306_Menu::setHovered(uint8_t index)
{
    if(_hovered == index)
        return;
    _hovered = index;
    _selected = false;
}

void OLED_SSD1306_Menu::select(bool select)
{
    _selected = select;
}

bool OLED_SSD1306_Menu::isSelected()
{
    return _selected;
}