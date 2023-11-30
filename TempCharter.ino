
/*
  Name:        TempCharter.ino
  Created:     10/11/2023 20:05 AM
  Author:      Martin Terner (github link goes here)
  Description: Arduino project for displaying and recording temperature, humidity and heat index data on an SSD1306 OLED display with wifi functionality.
*/

#include <OLED_SSD1306_Chart.h>
#include "OLED_SSD1306_Menu.h" //? made by myself
#include <Encoder.h>
#include <Wire.h>
#include <DHT.h>
#include <SD.h>

Encoder knob(1, 0);

#define DHTPIN 2
#define DHTTYPE DHT21 // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define BAUDRATE 9600

#define IP "192.168.0.124" // the Ip for the server on the lan(needs to be updated regularly unless a static ip is set up on the router or something to that effect)

#define DEBUGMODE true // whether or not to log anything to the serial monitor

OLED_SSD1306_Chart display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // initialize the oled display

// initialize the menus
OLED_SSD1306_Menu mainMenu(display, "TempCharter", 2, SCREEN_WIDTH, SCREEN_HEIGHT);
OLED_SSD1306_Menu reviewMenu(display, "review", 5, SCREEN_WIDTH, SCREEN_HEIGHT);
OLED_SSD1306_Menu reviewHumidityMenu(display, "review humidity", 101, SCREEN_WIDTH, SCREEN_HEIGHT);
OLED_SSD1306_Menu reviewTemperatureMenu(display, "review temperature", 101, SCREEN_WIDTH, SCREEN_HEIGHT);
OLED_SSD1306_Menu reviewHeatIndexMenu(display, "review heatIndex", 101, SCREEN_WIDTH, SCREEN_HEIGHT);
OLED_SSD1306_Menu reviewDataItemMenu(display, "review item", 3, SCREEN_WIDTH, SCREEN_HEIGHT);
OLED_SSD1306_Menu recordMenu(display, "record", 4, SCREEN_WIDTH, SCREEN_HEIGHT);

// store the menus in an array(as pointers) so that they can be selected as the currentMenu
OLED_SSD1306_Menu *menus[7] = {&mainMenu, &reviewMenu, &recordMenu, &reviewHumidityMenu, &reviewTemperatureMenu, &reviewHeatIndexMenu, &reviewDataItemMenu};

// enum for the currently rendered menu, used for readability.
enum class MenuType
{
    Main = 0,
    Review = 1,
    Record = 2,
    ReviewHumidity = 3,
    ReviewTemperature = 4,
    ReviewHeatIndex = 5,
    ReviewDataItem = 6,
};

MenuType currentMenuType = MenuType::Main;
MenuType previousMenuType = MenuType::Main;

const int buttonPin = 7; // the pin of the pushbutton

int buttonPressed = LOW;

int lastPosition = 0;

int hovered = 0;

int lastRealPosition = 0;

OLED_SSD1306_Menu *currentMenu;

OLED_SSD1306_Menu_Item *minutesButton;

OLED_SSD1306_Menu_Item *liveChartingButton;

OLED_SSD1306_Menu_Item *saveToButton;

OLED_SSD1306_Menu_Item *backButton;

char *measureOptions[3] = {"humidity", "temperature", "heatIndex"};
char *reviewFolderNames[3] = {"humidity", "temp", "heatIndx"}; //? has to be shorter because of FAT16 filename conventions
int measureIndex = 0;

const int chipSelect = SDCARD_SS_PIN;

char *viewLocation;
char *viewType;

/**
 * @brief Adds a button item for a file and sets the current menu to the review item menu.
 * @param menu pointer to the menu class to add the item to.
 * @param location char* string for the locations of the file.
 * @param type char* string for the type.
 * @param name char* string for the name of the file.
 */
void addReviewFileItem(OLED_SSD1306_Menu *menu, char *location, char *type, char *name)
{
    OLED_SSD1306_Menu_Item *item = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                              { setCurrentMenu(MenuType::ReviewDataItem); viewLocation = capture[0].c; viewType = capture[1].c; }, // sets the currentMenu to the review item menu and sets two global variables to the location and type of the data in the file.
                                                              name);

    Value *capture = new Value[2];

    capture[0].c = location;
    capture[1].c = type;

    item->setCapture(capture); // stores capture of the type and location which can be stored in global variables and then used when using the review item menu.

    menu->addItem(item);
}

/**
 * @brief Adds a button item for a file and sets the current menu to the review item menu.
 * @param menu pointer to the menu class to add the item to.
 * @param path char* string for the locations of the file.
 * @param name char* string for the name of the file.
 * @return Value struct with the value of the item.
 */
void populateMenu(OLED_SSD1306_Menu *menu, char *path, char *type)
{
    File dir = SD.open(path);
    File entry = dir.openNextFile();
    while (entry)
    {
        if (entry.isDirectory())
            continue;

        char *location = new char[25];

        char *filename = strdup(entry.name());

        sprintf(location, "%s/%s", path, filename);

        addReviewFileItem(menu, location, type, filename);

        entry.close();
        entry = dir.openNextFile();
    }
}

/**
 * @brief sets up the chart based on the measureIndex.
 * @param measureIndex int value for the measureIndex which indicates which type of data is being viewed(temp, heatIndex or humidity).
 */
void chartSetup(int measureIndex)
{
    display.clearDisplay();
    display.setChartCoordinates(0, 60);      // Chart lower left coordinates (X, Y)
    display.setChartWidthAndHeight(123, 55); // Chart width = 123 and height = 55
    display.setXIncrement(1);                // Distance between Y points will be 1px

    switch (measureIndex)
    {
    case 0:
        display.setYLimits(0, 100);          // Ymin = 0 and Ymax = 100
        display.setYLimitLabels("0", "100"); // Setting Y axis labels
        break;
    case 1:
        display.setYLimits(-40, 80);          // Ymin = 0 and Ymax = 100
        display.setYLimitLabels("-40", "80"); // Setting Y axis labels
        break;
    case 2:
        display.setYLimits(-40, 80);          // Ymin = 0 and Ymax = 100
        display.setYLimitLabels("-40", "80"); // Setting Y axis labels
        break;
    }

    display.setYLabelsVisible(true);
    display.setAxisDivisionsInc(3, 3); // Each 12 px a division will be painted in X axis and each 6px in Y axis
    display.setPointGeometry(POINT_GEOMETRY_NONE);
    display.setLineThickness(LIGHT_LINE);
    display.drawChart(); // Update the buffer to draw the cartesian chart
}

/**
 * @brief starts recording the data and displays it on the chart.
 * @param capture Value struct with some capture data(is not used by the function but is required by the menu library).
 */
void startRecording(Value *capture)
{
    int minutes = minutesButton->getValue().i;
    bool liveCharting = liveChartingButton->getValue().b;
    int saveTo = saveToButton->getValue().i;

    unsigned long chartingInterval = (minutes * 60 * 1000) / (unsigned long)(102);

    unsigned long recordingTime = 0;

    chartSetup(measureIndex);

    bool hasFinished = false;

    int storingTime = 0;

    int storeCount = 0;

    char *saveLocation = new char[25];

    char *filename = new char[10];

    sprintf(filename, "%d.csv", saveTo);

    sprintf(saveLocation, "data/%s/%s", reviewFolderNames[measureIndex], filename);

    File sensorData;

    sensorData = SD.open(saveLocation, FILE_WRITE | O_TRUNC);
    sensorData.close();
    sensorData = SD.open(saveLocation, FILE_WRITE);

    if (!sensorData)
    {
        Serial.println("writing to SD failed");
    }
    else
    {
        sensorData.println(minutes);      // metadata used to reconstruct the data when reviewing.
        sensorData.println(measureIndex); // metadata for the type of measuring used to reconstruct the data and also when sending with the esp.
    }

    for (;;)
    {
        float value = 0;

        switch (measureIndex)
        {
        case 0:
            value = dht.readHumidity();
            break;
        case 1:
            value = dht.readTemperature();
            break;
        case 2:
            value = dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity(), false);
            break;
        }

        unsigned long currentTime = millis();

        if (!hasFinished && currentTime > storingTime && sensorData)
        {
            storingTime = currentTime + 1000;

            sensorData.println(value);
        }

        if (currentTime > recordingTime && !hasFinished)
        {
            recordingTime = currentTime + chartingInterval;

            storeCount++;

            if (storeCount >= 102)
            {
                hasFinished = true;
            }
            if (liveCharting)
            {
                if (!display.updateChart(value)) // value between Ymin and Ymax will be added to chart
                {
                    hasFinished = true;
                }
            }
            else
            {
                display.clearDisplay();

                display.setCursor(0, 0);
                display.setTextColor(WHITE);

                display.print(String(storeCount) + "/" + String("102"));

                display.display();
            }
        }

        int button = digitalRead(buttonPin);

        uint8_t lastElementIndex;

        if (button == LOW && buttonPressed == HIGH)
        {
            switch (measureIndex)
            {
            case 0:
                addReviewFileItem(&reviewHumidityMenu, saveLocation, "humidity", filename); //? adds the file item to the menu
                lastElementIndex = reviewHumidityMenu.getItemCount() - 1;
                reviewHumidityMenu.swap(lastElementIndex - 1, lastElementIndex); //? swap the added item and the back buttons position
                break;
            case 1:
                addReviewFileItem(&reviewTemperatureMenu, saveLocation, "temperature", filename); //? adds the file item to the menu
                lastElementIndex = reviewTemperatureMenu.getItemCount() - 1;
                reviewTemperatureMenu.swap(lastElementIndex - 1, lastElementIndex); //? swap the added item and the back buttons position
                break;
            case 2:
                addReviewFileItem(&reviewHeatIndexMenu, saveLocation, "heatIndex", filename); //? adds the file item to the menu
                lastElementIndex = reviewHeatIndexMenu.getItemCount() - 1;
                reviewHeatIndexMenu.swap(lastElementIndex - 1, lastElementIndex); //? swap the added item and the back buttons position
                break;
            }
            sensorData.close();
            break;
        }

        buttonPressed = button;
    }
}

/**
 * @brief sends file to a web server hosted on LAN based on the IP constant and inputs(esp needs to be connected to wifi).
 * @param file File class with the file to send.
 * @param type char* string for the type of data(temperature, heatIndex or humidity).
 */
void sendRecordingThroughESP(File file, String type)
{
    if (!file.available() && DEBUGMODE)
    {
        Serial.println("file could not be found!");
        return;
    }

    String data = "{\"type\":\"" + type + "\",\"data\":[";

    int index = 0;

    while (file.available())
    {
        index++;
        if (index == 0)
        {
            int data2 = file.parseInt();
            continue;
        }

        if (index == 1)
        {
            int data2 = file.parseInt();
            continue;
        }

        int data2 = file.parseFloat();

        data += String(data2);
        data += ",";
    }
    file.close();

    data.remove(data.length() - 1);

    data += "]}";

    if (DEBUGMODE)
        Serial.println(data);

    Serial1.print("AT+CIPSTART=\"TCP\",\"" + String(IP) + "\",3000\r\n"); // connects the esp to the ip and port of the lan server, all qoutes are escaped with backslashes
    while (!(Serial1.available() > 0))
    {
    }
    delay(4000);
    logOutput();
    Serial1.print("AT+CIPSEND=" + String(data.length() + 103) + "\r\n"); // the total amount of bytes for the request
    delay(8000);
    logOutput();

    Serial1.println("POST /send HTTP/1.1");
    delay(8000);
    logOutput();
    Serial1.println("Host: " + String(IP) + ":3000");
    delay(8000);
    logOutput();
    Serial1.println("Content-Type: application/json");
    delay(8000);
    logOutput();
    Serial1.println("Content-Length: " + String(data.length())); // length of the json data to be sent
    delay(8000);
    Serial1.println();
    logOutput();
    Serial1.println(data); // json data
    delay(8000);
    logOutput();
}

/**
 * @brief logs the output of the esp serial to the serial monitor.
 */
void logOutput(void)
{
    if (!DEBUGMODE)
        return;
    while (Serial1.available() > 0)
    {
        Serial.print((char)Serial1.read());
    }
}

/**
 * @brief reads the data in the provided file(must be csv with correct format to work as intended) and displays it on the oled screen.
 * @param file File class with the file to view.
 */
void viewRecording(File file)
{
    int minutes;
    int skipInterval;
    int skipCount;
    int measureIndex;
    int index = -1;

    if (!file.available() && DEBUGMODE)
    {
        Serial.println("file could not be found!");
        return;
    }

    while (file.available())
    {

        index++;
        if (index == 0)
        {
            int data = file.parseInt();
            minutes = data;
            skipInterval = floor((minutes * 60) / (102));
            skipCount = skipInterval + 1;
            continue;
        }

        if (index == 1)
        {
            int data = file.parseInt();
            measureIndex = data;
            chartSetup(measureIndex);
            continue;
        }

        skipCount--;

        float data = file.parseFloat();

        if (skipCount > 0)
            continue;

        if (DEBUGMODE)
            Serial.println(data);

        skipCount = skipInterval + 1;

        if (!display.updateChart(data))
        {
            break;
        }
    }
    file.close();

    buttonPressed = LOW;
    while (true)
    {
        int button = digitalRead(buttonPin);

        if (button == LOW && buttonPressed == HIGH)
        {
            break;
        }

        buttonPressed = button;
    }
}

/**
 * @brief convenient function for changing the currently displayed/used menu.
 * @param type the MenuType.
 */
void setCurrentMenu(MenuType type)
{
    previousMenuType = currentMenuType;
    currentMenuType = type;
    currentMenu->select(false);
}

int lastKnobRead = 0;
/**
 * @brief convenient function for changing a selected float value in a menu.
 * @param value the value to change.
 * @param min the minimum value.
 * @param max the maximum value.
 * @param capture Value struct with some capture data(is not used by the function but is required by the menu library).
 */
float inputFloatUpdate(float value, float min, float max, Value *capture)
{
    int knobRead = floor(knob.read() / 4.0f);

    if (lastKnobRead == 0)
        lastKnobRead = knobRead;

    int newValue = value + (knobRead - lastKnobRead);

    lastKnobRead = knobRead;

    return constrain(newValue, min, max);
}

/**
 * @brief convenient function for changing a selected int value in a menu.
 * @param value the value to change.
 * @param min the minimum value.
 * @param max the maximum value.
 * @return the updated value
 */
int inputIntUpdate(int value, int min, int max)
{
    int knobRead = floor(knob.read() / 4.0f);

    if (lastKnobRead == 0)
        lastKnobRead = knobRead;

    int newValue = value + (knobRead - lastKnobRead);

    lastKnobRead = knobRead;

    return constrain(newValue, min, max);
}

/**
 * @brief convenient function for changing a selected bool value in a menu.
 * @param capture Value struct with some capture data(is not used by the function but is required by the menu library).
 * @return the updated value
 */
bool inputBoolUpdate(bool value, Value *capture)
{
    currentMenu->select(false);

    return !value;
}

/**
 * @brief convenient function for back buttons in menus.
 * @param capture Value struct with some capture data(is not used by the function but is required by the menu library).
 */
void backButtonUpdate(Value *capture)
{
    currentMenu->select(false);
    switch (currentMenuType)
    {
    case MenuType::Main:
    case MenuType::Review:
    case MenuType::Record:
        currentMenuType = MenuType::Main;
        break;
    case MenuType::ReviewHumidity:
    case MenuType::ReviewTemperature:
    case MenuType::ReviewHeatIndex:
        currentMenuType = MenuType::Review;
        break;
    case MenuType::ReviewDataItem:
        currentMenuType = previousMenuType;
        break;
    default:
        currentMenuType = MenuType::Main;
        break;
    }
}

/**
 * @brief convenient function for buttons that makes them do absolutely nothing, used primarily for debugging/prototyping.
 * @param capture Value struct with some capture data(is not used by the function but is required by the menu library).
 */
void prototypingFunction(Value *capture)
{
    currentMenu->select(false);
}

/**
 * @brief function for going through the different measure options in the record menu.
 * @param value char* string with the current measure option(unused but necessary as it is provided by the menu library).
 * @param capture Value struct with some capture data(is not used by the function but is required by the menu library).
 */
char *measureOptionGoThrough(char *value, Value *capture)
{
    measureIndex = inputIntUpdate(measureIndex, 0, 2);
    return measureOptions[measureIndex];
}

/**
 * @brief function that initializes all the menus and adds all their buttons and functionality.
 */
void setupMenus()
{
    OLED_SSD1306_Menu_Item *reviewSubmenuButton = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                                             { setCurrentMenu(MenuType::Review); }, // currentMenuType = 1; mainMenu.select(false);
                                                                             "review");

    OLED_SSD1306_Menu_Item *recordSubmenuButton = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                                             { setCurrentMenu(MenuType::Record); }, // currentMenuType = 2; mainMenu.select(false);
                                                                             "record");

    OLED_SSD1306_Menu_Item *inputOption = new OLED_SSD1306_Menu_Item(&measureOptionGoThrough, measureOptions[0], "measure");

    OLED_SSD1306_Menu_Item *startRecordingButton = new OLED_SSD1306_Menu_Item(&startRecording, "start recording");
    minutesButton = new OLED_SSD1306_Menu_Item([](int value, Value *capture) -> int
                                               { return inputIntUpdate(value, 1, 999); },
                                               60, "minutes");
    saveToButton = new OLED_SSD1306_Menu_Item([](int value, Value *capture) -> int
                                              { return inputIntUpdate(value, 0, 99); },
                                              0, "save to");
    liveChartingButton = new OLED_SSD1306_Menu_Item(&inputBoolUpdate, true, "live charting");

    OLED_SSD1306_Menu_Item *reviewHumidity = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                                        { setCurrentMenu(MenuType::ReviewHumidity); }, // currentMenuType = 3; currentMenu->select(false); previousMenuType=3;
                                                                        "humidity");
    OLED_SSD1306_Menu_Item *reviewTemperature = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                                           { setCurrentMenu(MenuType::ReviewTemperature); }, // currentMenuType = 4;previousMenuType = 4;currentMenu->select(false);
                                                                           "temperature");
    OLED_SSD1306_Menu_Item *reviewHeatIndex = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                                         { setCurrentMenu(MenuType::ReviewHeatIndex); }, // currentMenuType = 5; previousMenuType = 5; currentMenu->select(false);
                                                                         "heatIndex");
    backButton = new OLED_SSD1306_Menu_Item(&backButtonUpdate, "back");

    populateMenu(&reviewHumidityMenu, "/data/humidity", "humidity");
    reviewHumidityMenu.addItem(backButton);

    populateMenu(&reviewTemperatureMenu, "/data/temp", "temperature");
    reviewTemperatureMenu.addItem(backButton);

    populateMenu(&reviewHeatIndexMenu, "/data/heatIndx", "heatIndex");
    reviewHeatIndexMenu.addItem(backButton);

    OLED_SSD1306_Menu_Item *viewButton = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                                    { viewRecording(SD.open(viewLocation)); },
                                                                    "view");
    OLED_SSD1306_Menu_Item *sendToDeviceButton = new OLED_SSD1306_Menu_Item([](Value *capture) -> void
                                                                            { sendRecordingThroughESP(SD.open(viewLocation), viewType); currentMenu->select(false); },
                                                                            "send to device");
    reviewDataItemMenu.addItem(viewButton);
    reviewDataItemMenu.addItem(sendToDeviceButton);
    reviewDataItemMenu.addItem(backButton);

    reviewMenu.addItem(reviewHumidity);
    reviewMenu.addItem(reviewTemperature);
    reviewMenu.addItem(reviewHeatIndex);
    reviewMenu.addItem(backButton);

    recordMenu.addItem(startRecordingButton);
    recordMenu.addItem(minutesButton);
    recordMenu.addItem(liveChartingButton);
    recordMenu.addItem(inputOption);
    recordMenu.addItem(saveToButton);
    recordMenu.addItem(backButton);

    mainMenu.addItem(reviewSubmenuButton);
    mainMenu.addItem(recordSubmenuButton);
}

void setup()
{
    if (DEBUGMODE)
        Serial.begin(BAUDRATE);
    Serial1.begin(BAUDRATE); //? Serial port for RX and TX which lets me communicate with the esp8266 on the mkrzero.

    dht.begin();

    display.begin(SSD1306_SWITCHCAPVCC, 0x3c);

    display.clearDisplay();

    display.setCursor(0, 0);
    display.setTextColor(WHITE);

    display.print("awaiting Serial"); // will barely be visible if DEBUGMODE is off

    display.display();

    while (!Serial && DEBUGMODE)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    while (!Serial1 && DEBUGMODE)
    {
        ;
    }

    if (DEBUGMODE)
        Serial.println("Initializing SD card...");

    if (!SD.begin(chipSelect))
    {

        if (DEBUGMODE)
            Serial.println("initialization failed");

        while (true)
            ;
    }

    display.clearDisplay();

    setupMenus();

    mainMenu.update();

    pinMode(buttonPin, INPUT);

    mainMenu.update();
}

/**
 * @brief logs the output of Serial1 which the esp uses to the Serial and vice versa.
 */
void logEspOutput()
{
    if (Serial1.available() > 0)
    {
        Serial.print((char)Serial1.read());
    }
    if (Serial.available() > 0)
    {
        char chr = Serial.read();
        Serial1.print(chr);
    }
}

void loop()
{
    //? debug the Serial1 port(for the esp8266)
    if (DEBUGMODE)
    {
        logEspOutput();
    }

    currentMenu = menus[static_cast<int>(currentMenuType)]; //? set the current menu being rendered and navigated based on the currentMenuType.

    int position = floor(knob.read() / 4.0f);

    int itemCount = currentMenu->getItemCount();

    if (!currentMenu->isSelected())
    {
        currentMenu->setHovered(position < 0 ? itemCount - 1 - (abs(position + 1) % itemCount) : position % itemCount); //? very janky way to traverse the menu based on the position read using the encoder library
        lastRealPosition = knob.read();                                                                                 //? stores the position so that it can be saved and let the selected menu item be the same even if the menu item uses the encoder(like most do except for buttons)
    }

    int button = digitalRead(buttonPin);

    if (digitalRead(buttonPin) == LOW && buttonPressed == HIGH) //? if the button goes from being pressed to not being pressed(if the button is clicked)
    {
        currentMenu->select(!currentMenu->isSelected()); // select/unselect the currently hovered item

        if (!currentMenu->isSelected())
        {
            knob.write(lastRealPosition);
        }
    }
    buttonPressed = button;

    currentMenu->update(); //? render the current menu.

    lastKnobRead = position;
}