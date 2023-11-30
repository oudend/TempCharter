#ifndef OLED_SSD1306_MENU_H
#define OLED_SSD1306_MENU_H

#include <Adafruit_SSD1306.h>
// #include <Wire.h>

union Value {
    float f;
    int i;
    bool b;
    char* c;
};

class OLED_SSD1306_Menu_Item
{

    union Update {
        float (*f)(float, Value*);
        int (*i)(int, Value*);
        bool (*b)(bool, Value*);
        char* (*c)(char*, Value*);
        void (*v)(Value*);
    };

    enum Type
    {
        Button = 0,
        Float = 1,
        Bool = 2,
        Int = 3,
        Char = 4
    };

    private:

        Type _type; //it's still a Type enum more or less
        Value _value;
        Update _update;
        char* _label;
        Value* _capture;

    public:
        /**
        * @brief Renders the item to the oled. 
        * @param *display - pointer to the oled display of type Adafruit_SSD1306.
        * @param selected - an boolean indicating whether or not the item is selected.
        * @param hovered - 
        * @param *display - pointer to the oled display of type Adafruit_SSD1306.
        */
        void draw(Adafruit_SSD1306 *display, bool selected, bool hovered, int y, int width);
        void update(bool selected);
        /**
        * @brief Gets the current value stored in the menu in the form of the Value struct(value.c = char* value, value.i = int value, value.f = float value, value.b = bool value).
        * @return Value struct with the value of the item.
        */
        Value getValue();

        /**
        * @brief Sets the capture variable of the menu item which can be accessed on callbacks.
        * @param *capture - pointer to a Value struct variable with the capture data.
        */
        void setCapture(Value* capture);
        
        /**
        * @brief Returns a clone of the current item.
        * @return OLED_SSD1306_Menu_Item pointer to the clone.
        */
        virtual OLED_SSD1306_Menu_Item* clone() const;

        // variables that determine the blinking of the dot that indicates that the item is being controlled
        float selectedBlinkDelay = 650.0f;
        float selectedBlinkDuration = 700.0f;
        float lastDelay = 0.0f;

        OLED_SSD1306_Menu_Item(float (*update)(float, Value*), float value, char* label); // float item
        OLED_SSD1306_Menu_Item(int (*update)(int, Value*), int value, char* label); // integer item
        OLED_SSD1306_Menu_Item(char* (*update)(char*, Value*), char* value, char* label); // char* item
        OLED_SSD1306_Menu_Item(bool (*update)(bool, Value*), bool value, char* label); // boolean item
        OLED_SSD1306_Menu_Item(void (*update)(Value*), char* value); // button item
};

/**
* @brief Creates an instance of OLED_SSD1306_Menu.
* @param &display - pointer to the oled display of type Adafruit_SSD1306.
* @param menuLabel - the label of the menu which will be displayed at the top when rendered.
* @param items - the amount of items that will be added to the menu(high numbers may cause memory errors).
* @param width - the width of the display.
* @param height - the height of the display.
*/
class OLED_SSD1306_Menu
{
    private:
        OLED_SSD1306_Menu_Item** _items; //the variable that stores all the items
        Adafruit_SSD1306* _display;
        uint8_t _itemCount = 0;
        uint8_t _hovered;
        int _width;
        int _height;
        bool _selected;
        void _draw(); //clear the entire screen and redraw menu centered around selected input type
        int calculateInitialOffset(); //calculates the initial offset of the menu before it gets rendered, this enables simple scrolling.

    public:
        char* label;

        /**
        * @brief Draws a center aligned string.
        * @param &display - pointer to the oled display of type Adafruit_SSD1306.
        * @param *buf - the char* that will be rendered. 
        * @param width - the width of the display.
        * @param y - the y position of the string.
        */
        static void drawCentreString(Adafruit_SSD1306 *display, const char *buf, uint8_t x, uint8_t y);
        /**
        * @brief Draws a right aligned string.
        * @param &display - pointer to the oled display of type Adafruit_SSD1306.
        * @param *buf - the char* that will be rendered. 
        * @param width - the width of the display.
        * @param y - the y position of the string.
        */
        static void drawRightAlignedString(Adafruit_SSD1306 *display, const char *buf, uint8_t width, uint8_t y);

        OLED_SSD1306_Menu(Adafruit_SSD1306 &display, char* menuLabel, uint8_t items = 10, int width = 128, int height = 64)
        {
            _items = new OLED_SSD1306_Menu_Item*[items];
            _display = &display;
            _hovered = 0;
            _selected = false;
            label = menuLabel;
            _width = width;
            _height = height;
        };

        /**
        * @brief Clears the menu and all its items(will delete all the items)
        * @param index2 - the index  of the second item to be swapped with the first
        */
        void clear();

        /**
        * @brief Swaps two items in the menu based on their indices.
        * @param index1 - the index of the first item to be swapped with the second
        * @param index2 - the index  of the second item to be swapped with the first
        */
        void swap(int index1, int index2);

        /**
        * @brief Returns whether or not anything is selected
        * @param index - index for what item should be selected(from 0 to itemCount - 1)
        */
        void setHovered(uint8_t index);
        /**
        * @brief Selects the currently hovered item
        * @param select - whether or not the element should be selected(has no effect if already the same)
        */
        void select(bool select);
        /**
        * @brief Returns whether or not anything is selected
        * @return selected
        */
        bool isSelected();

        /**
        * @brief Returns the current count of items
        * @return The item count
        */
        uint8_t getItemCount();

        /**
        * @brief Adds the inputted item to the end of the menu
        * @param *item a pointer to the item to be added to the menu
        */
        void addItem(OLED_SSD1306_Menu_Item *item);

        /**
        * @brief Updates the  menu and redraws it
        */
        void update();
};

#endif