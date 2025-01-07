#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <GyverPortal.h>
#include <EEPROM.h>
#include <EEManager.h>
#include <LittleFS.h>
#include <PairsFile.h>
#include <FastBot2.h>

void formatLittleFS();
void defaultConfig();
void printConfig();
void openGate1();
void openGate3();
void startAP();
void updateGates();
void updateh(fb::Update& u);

PairsFile config(&LittleFS, "/config.dat", 3000);
GyverPortal ui;
FastBot2 bot;

// Компоненты страницы
//GP_LABEL_BLOCK lbl("lbl");
GP_LABEL lblWiFi("lblWiFi");
GP_LABEL lblTx("lblTx");
GP_LABEL lblRx("lblRx");
GP_LABEL lblSave("lblSave");
GP_LABEL lblToken("lblToken");
GP_BUTTON btnSaveWifi("saveWifi", "Сохранить настройки");
GP_BUTTON btnSaveConfig("saveConfig", "Сохранить настройки");
GP_BUTTON btnRefresh("refresh", "Обновить сети");
GP_BUTTON_MINI btnDel("btnDel", "Забыть сеть");
GP_PASS   passField("pass", "Пароль", "", "100%");
GP_TEXT   txtToken("txtToken", "Токен телеграмм бота", "", "100%");
GP_SELECT wifiSelect("wifi", "Доступные сети");
GP_SELECT GPIO_Gates_1("GPIO_Gates_1");
GP_SELECT GPIO_Gates_3("GPIO_Gates_3");
GP_SELECT GPIO_Led("GPIO_Led");
GP_BUTTON btnOpen1("btnOpen1", "Открыть ворота 1п");
GP_BUTTON btnOpen3("btnOpen3", "Открыть ворота 3п");
GP_BUTTON btnLittleFS("btnLittleFS", "format LittleFS");
GP_BUTTON btnDefConfig("btnDefConfig", "defult Config");
GP_BUTTON btnPrintConfig("btnPrintConfig", "print Config");
GP_SPINNER openThreshold("openThreshold", 2.0, 0.1, 5.0, 0.1, 1, GP_GREEN);
GP_SPINNER timeDownBtn("timeDownBtn", 1000,  50, 9999, 50, 0, GP_GREEN, "72px");

#define AP_SSID "Gates"
#define AP_PASS "12345678"
#define BOT_TOKEN "5313332238:AAH_y7CKD9rovDF71Wmjke0K1uB58nAx064"
#define CHAT_ID "-4680217636"
#define CONNECT_TIMEOUT 900000 // 15 минут таймаут подключения
String tmpSSID = ""; // Промежуточная переменная для хранения выбранного SSID
String tmpPass = ""; // 
float tmpFloat;
float tmpInt;
bool ledState = true;  // Состояние светодиода
bool update = false;
bool isWiFi = false;
bool isOpenGate1 = false;
bool isOpenGate3 = false;
bool isSave = false;
unsigned long startTime;

#ifdef ESP8266
// Массив с названиями пинов и их значениями
const char* pinNames[] = {"D0 (GPIO16)", "D1 (GPIO5)", "D2 (GPIO4)", "D3 (GPIO0)", "D4 (GPIO2)", "D5 (GPIO14)", "D6 (GPIO12)", "D7 (GPIO13)", "D8 (GPIO15)"};
const int pinValues[] = {16, 5, 4, 0, 2, 14, 12, 13, 15};
const int numPins = sizeof(pinNames) / sizeof(pinNames[0]);
const char* adcNames[] = {"A0"};
const int adcValues[] = {0}; // Значение не важно, так как только один вход
const int numAdc = sizeof(adcNames) / sizeof(adcNames[0]);
#endif
#ifdef ESP32
const char* pinNames[] = {"GPIO0", "GPIO1", "GPIO2", "GPIO3", "GPIO4", "GPIO5", "GPIO12", "GPIO18", "GPIO19", "GPIO20", "GPIO21"};
const int pinValues[] = {0, 1, 2, 3, 4, 5, 12, 18, 19, 20, 21};
const int numPins = sizeof(pinNames) / sizeof(pinNames[0]);
const char* adcNames[] = {"GPIO0 (ADC1_CH0)", "GPIO1 (ADC1_CH1)", "GPIO2 (ADC1_CH2)", "GPIO3 (ADC1_CH3)", "GPIO4 (ADC1_CH4)", "GPIO5 (ADC1_CH5)"};
const int adcValues[] = {0, 1, 2, 3, 4, 5};
const int numAdc = sizeof(adcNames) / sizeof(adcNames[0]);
#endif


#define STATIC_IP IPAddress(192, 168, 0, 111)
#define GATEWAY IPAddress(192, 168, 0, 1)
#define SUBNET IPAddress(255, 255, 255, 0)

void updateh(fb::Update& u) {
    Serial.println("NEW MESSAGE");
    Serial.println(u.message().from().username());
    Serial.println(u.message().text());

    //if(u.message().text() == "1" || u.message().text() == "1п" || u.message().text() == "Ворота 1п") {
    //  bot.sendMessage(fb::Message("Открываю 1п", u.message().chat().id()));
    //}

    // #1
    // отправить обратно в чат (эхо)
    bot.sendMessage(fb::Message(u.message().text(), u.message().chat().id()));

    // #2
    // декодирование Unicode символов (кириллицы) делается вручную!
    // String text = u.message().text().decodeUnicode();
    // text += " - ответ";
    // bot.sendMessage(fb::Message(text, u.message().chat().id()));

    // #3
    // или так

    //if(msg.text == "1" || msg.text == "1п" || msg.text == "Ворота 1п") {
    //  msg.text = "Открываю 1п";
    //  bot.sendMessage(msg);
    //}
    //if(msg.text == "3" || msg.text == "3п" || msg.text == "Ворота 3п") {
    //  msg.text = "Открываю 3п";
    //  bot.sendMessage(msg);
    //}

}

void printWiFiMode() {
    WiFiMode_t mode = WiFi.getMode();
    Serial.print("Текущий режим Wi-Fi: ");
    
    switch (mode) {
        case WIFI_OFF:
            Serial.println("Wi-Fi выключен");
            break;
        case WIFI_STA:
            Serial.println("Режим клиента (STA)");
            break;
        case WIFI_AP:
            Serial.println("Режим точки доступа (AP)");
            break;
        case WIFI_AP_STA:
            Serial.println("Режим AP+STA");
            break;
        default:
            Serial.println("Неизвестный режим");
            break;
    }
}

void buildPortal() {
    GP.BUILD_BEGIN();
    GP.THEME(GP_DARK);
    GP.UPDATE("ledTx, ledRx, ledWifi, ledSave"); // список имён компонентов на обновление
    GP.BOX_BEGIN(GP_JUSTIFY);
        GP.LABEL("WIFI: ","lblWiFi");
        GP.LED("ledWifi", bool(WiFi.getMode() == WIFI_STA), GP_GREEN);
        GP.LABEL("Save: ","lblSave");
        GP.LED("ledSave", isSave, GP_YELLOW);
        GP.BOX_BEGIN(GP_CENTER, "80%");  
        GP.BOX_END();
        GP.BOX_BEGIN(GP_CENTER); 
            GP.LABEL("TX: ","lblTx");
            GP.LED("ledTx", ledState, GP_GREEN);
            GP.LABEL("RX: ","lblRx");
            GP.LED("ledRx", bool(WiFi.getMode() == WIFI_STA), GP_RED);
        GP.BOX_END();
    GP.BOX_END();
    GP.BREAK();
    GP.NAV_TABS("Main,Settings,Wifi,Log");

    GP.NAV_BLOCK_BEGIN();
    GP.BOX_BEGIN(GP_CENTER);
    GP.BUTTON(btnOpen1);
    GP.BOX_END();
    GP.BUTTON(btnOpen3);
    GP.NAV_BLOCK_END();

    GP.NAV_BLOCK_BEGIN();
      GP.BOX_BEGIN(GP_RIGHT);
        GP.LABEL("1п GPIO");
        GP.SELECT(GPIO_Gates_1);
      GP.BOX_END();
      GP.BOX_BEGIN(GP_RIGHT);
        GP.LABEL("3п GPIO");
        GP.SELECT(GPIO_Gates_3);
      GP.BOX_END();
      GP.BOX_BEGIN(GP_RIGHT);
        GP.LABEL("Led GPIO");
        GP.SELECT(GPIO_Led);
      GP.BOX_END();
      GP.BOX_BEGIN(GP_RIGHT);
        GP.LABEL("Длит. нажатия");
        GP.SPINNER("timeDownBtn", config["timeDownBtn"],  50, 9999, 50, 0, GP_GREEN, "72px"); GP.BREAK();
      GP.BOX_END();
      GP.BOX_BEGIN(GP_RIGHT);
        GP.LABEL("Порог откр. Vt");
        GP.SPINNER("openThreshold", config["openThreshold"], 0.1, 5.0, 0.1, 1, GP_GREEN, "72px"); GP.BREAK();
      GP.BOX_END();
      GP.BOX_BEGIN(GP_RIGHT);
        GP.LABEL("BotToken");
        GP.TEXT("txtToken", config["botToken"]);
      GP.BOX_END();
    GP.NAV_BLOCK_END();

    GP.NAV_BLOCK_BEGIN();
      if (WiFi.getMode() == WIFI_AP_STA) {
          GP.BOX_BEGIN(GP_CENTER, "100%"); 
              GP.SELECT(wifiSelect);
          GP.BOX_END();
          GP.PASS(passField);
          GP.BUTTON(btnSaveWifi); 
      } else {
          GP.BOX_BEGIN(GP_RIGHT);
              GP.LABEL("SSID: " + String(WiFi.SSID()));     GP.BREAK();
          GP.BLOCK_END();
          GP.BOX_BEGIN(GP_RIGHT);
              GP.LABEL("dBm: " + String(WiFi.RSSI()));       GP.BREAK();
          GP.BLOCK_END();
          GP.BOX_BEGIN(GP_RIGHT);
              GP.LABEL("IP: " + WiFi.localIP().toString()); GP.BREAK();
          GP.BLOCK_END();
          GP.CONFIRM("confirm_delete", "Вы действительно хотите удалить сеть? Устройство будет перезагружено!");
          GP.BUTTON_MINI(btnDel);
          GP.UPDATE_CLICK("confirm_delete", "btnDel");
      }
    GP.NAV_BLOCK_END();

    GP.NAV_BLOCK_BEGIN();
    GP.BUTTON(btnDefConfig); // Сброс настроек к заводским
    GP.BUTTON(btnLittleFS); //Форматирование памяти
    GP.BUTTON(btnPrintConfig); // Печать настроек
    GP.NAV_BLOCK_END();

    GP.BUILD_END();
}

void scanWiFi() {
    Serial.println("Сканируем сети...");
    int networks = WiFi.scanNetworks();
    String options = "";
    for (int i = 0; i < networks; i++) {
        options += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";// Добавляем SSID в список
        Serial.println(WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)");
        if (i < networks - 1) options += ",";
    }
    wifiSelect.list = options;  // Передаем строку с SSID
    //Serial.println("options: " + options);
}

void action() {
    if (ui.click()) {
        if (ui.click(btnSaveWifi)) {
            Serial.println("Сохраняем настройки и перезагружаемся.");
            if (tmpSSID.length() == 0) {
                tmpSSID = wifiSelect.getValue();
                // Удаление уровня сигнала (убираем всё, начиная с пробела и круглой скобки)
                int delimiterIndex = tmpSSID.indexOf(" (");
                if (delimiterIndex != -1) {
                    tmpSSID = tmpSSID.substring(0, delimiterIndex);
                }
                Serial.println("Выбранный SSID: " + tmpSSID);
            }
            config["ssid"] = tmpSSID;
            config["password"] = tmpPass;
            config.update();
            Serial.print("ssid: ");
            Serial.println(config["ssid"]);
            Serial.print("password: ");
            Serial.println(config["password"]);
            if (config["ssid"].length()>0) {
              Serial.println("Перезагружаемся...");
              ESP.restart();
            }
        }
        if (ui.click(btnLittleFS)) {
            Serial.println("LittleFS format");
            formatLittleFS();
        }
        if (ui.click(btnDefConfig)) {
            Serial.println("default Config");
            defaultConfig();
        }
        if (ui.click(btnPrintConfig)) {
            Serial.println("print Config");
            printConfig();
            bot.sendMessage(fb::Message("Hello!", -4680217636));
        }
        //ui.clickInt("GPIO_Gates_1", config.GPIO_Gates_1);

        if(ui.clickInt("GPIO_Gates_3", tmpInt)) {
            config["GPIO_Gates_3"] = tmpInt;
            config.update();
        } // сохраняем значение пина 3 ворот
        if(ui.clickInt("GPIO_Led", tmpInt)) {
            config["GPIO_Led"] = tmpInt;
            config.update();
          } // сохраняем значение пина светодиода
        if(ui.clickInt("timeDownBtn", tmpInt)) {
            config["timeDownBtn"] = tmpInt;
            config.update();
        } // сохраняем значение времени удержания
        if (ui.clickFloat("openThreshold", tmpFloat)) {
            config["openThreshold"] = tmpFloat;
            config.update();
        }

        if (ui.click(wifiSelect)) {
            // Обновляем промежуточную переменную при выборе элемента
            tmpSSID = wifiSelect.getValue();
            // Удаление уровня сигнала (убираем всё, начиная с пробела и круглой скобки)
            int delimiterIndex = tmpSSID.indexOf(" (");
            if (delimiterIndex != -1) {
                tmpSSID = tmpSSID.substring(0, delimiterIndex);
            }
            Serial.println("Выбранный SSID обновлён: " + tmpSSID);

        }
        if (ui.click(GPIO_Gates_1)) {
            config["GPIO_Gates_1"] = pinValues[GPIO_Gates_1.selected];
            config.update();
            Serial.print("Выбранный пин: ");
            Serial.print(config["GPIO_Gates_1"]);
            Serial.print(" - ");
            Serial.println(GPIO_Gates_1.getValue());
        }
        if (ui.click(GPIO_Gates_3)) {
            config["GPIO_Gates_3"] = pinValues[GPIO_Gates_3.selected];
            config.update();
            Serial.print("Выбранный пин: ");
            Serial.print(config["GPIO_Gates_3"]);
            Serial.print(" - ");
            Serial.println(GPIO_Gates_3.getValue());
        }

        if (ui.click(GPIO_Led)) {
            config["GPIO_Led"] = adcValues[GPIO_Led.selected];
            config.update();
            Serial.print("Выбранный пин: ");
            Serial.print(config["GPIO_Led"]);
            Serial.print(" - ");
            Serial.println(GPIO_Led.getValue());
        }

        if (ui.click(passField)) {
            tmpPass = passField.text;
            Serial.println("Поле пароля: " + tmpPass);
        }

        if (ui.click(txtToken)) {
            config["botToken"] = txtToken.text;
            config.update();
            Serial.print("Поле токена: ");
            Serial.println(config["botToken"]);
        }

        if (ui.click("confirm_delete")) {
            Serial.println(String("confirm_delete: ") + ui.getBool());
            if (ui.getBool()) {
                Serial.println("Забываем точку доступа и пароль.");
                config["ssid"] = "";
                config["password"] = "";
                WiFi.disconnect(true, true); // Очищает сохранённый SSID и пароль
                config.update();
                delay(1000);
                ESP.restart();
            } else {
               Serial.println("Отмена удаления сети.");
            }
        }
        if (ui.click("btnOpen1")) {
          Serial.println("Открываем ворота 1п...");
          delay(100);
          openGate1();
        }
        if (ui.click("btnOpen3")) {
          Serial.println("Открываем ворота 3п...");
          delay(100);
          openGate3();
        }
        
    }
    if (ui.update()) {
        // вызов с кнопки
        if (ui.update("confirm_delete")) {
          ui.answer(1);
        }
        if (ui.update("ledWifi")) ui.answer(isWiFi);
        if (ui.update("ledSave")) {
            ui.answer(isSave);
            isSave = false;
        };
        //ui.updateBool("ledTx", ledState);
    }
}
void connectWiFi() {
    WiFi.config(STATIC_IP, GATEWAY, SUBNET);
    Serial.print("Статический IP: ");
    Serial.println(WiFi.localIP());
    WiFi.begin(config["ssid"], config["password"]);
    Serial.print("Подключение к WiFi: ");
    Serial.println(config["ssid"]);
    Serial.print("Пароль: ");
    Serial.println(config["password"]);

    unsigned long start = millis(); // запоминаем время начала подключения
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < CONNECT_TIMEOUT) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Подключено!");
        Serial.println("Подключено к " + String(WiFi.SSID())+ " (" + String(WiFi.RSSI()) + ") " + " (IP: " + WiFi.localIP().toString() + ")");
        ui.attachBuild(buildPortal);
        ui.attach(action);
        ui.start();
    } else {
        Serial.println("Не удалось подключиться. Запуск AP...");
        WiFi.disconnect(true, true);
        startAP();
    }
}

void startAP() {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    scanWiFi();
    ui.attachBuild(buildPortal);
    ui.attach(action);
    ui.start();
    Serial.println("AP запущена. IP: " + WiFi.softAPIP().toString());
    printWiFiMode();
}

void setup() {
  Serial.begin(115200);
  //LittleFS.begin();
  delay(1000);
  if (LittleFS.begin()) {
      Serial.println("LittleFS загружен");  
    } else {
      Serial.println("Ошибка загрузки LittleFS");
    }
  if (config.begin()) {
      Serial.println("Файл с настройками загружен: ");
      printConfig();
  } else {
      Serial.println("Создаем файл с значениями по умолчанию и загружаем его...");
      defaultConfig();
  }
     // прочитать из файла
  //EEPROM.begin(EEPROM_SIZE);
  //byte status = memory.begin(0, 'y');
  // Проверем переменную GPIO_Gates_1 если она пустая выставляем значения по умолчанию
  if (config["GPIO_Gates_1"].length() == 0 || config["GPIO_Gates_3"].length() == 0 ) {
      defaultConfig();
      if (config["GPIO_Gates_1"].length() > 0) pinMode(config["GPIO_Gates_1"], OUTPUT);
      if (config["GPIO_Gates_3"].length() > 0) pinMode(config["GPIO_Gates_3"], OUTPUT);
  } 
  // Проверяем статус
  if (config["ssid"].length() == 0) {
        Serial.println("SSID пустой, запускаемся в режиме AP");
        startAP();
      } else {
        printConfig();
        Serial.println("SSID найден, подключаемся к WiFi" + config["ssid"]);
        connectWiFi();
      }
  

  // Настройка выпадающего списка
  GPIO_Gates_1.list = "";
  for (int i = 0; i < numPins; i++) {
    GPIO_Gates_1.list += pinNames[i];
    if (i < numPins - 1) {
      GPIO_Gates_1.list += ",";
    }
  }
  // Настройка выпадающего списка
  GPIO_Gates_3.list = "";
  for (int i = 0; i < numPins; i++) {
    GPIO_Gates_3.list += pinNames[i];
    if (i < numPins - 1) {
      GPIO_Gates_3.list += ",";
    }
  }
// Настройка выпадающего списка
   GPIO_Led.list = "";
    for (int i = 0; i < numAdc; i++) {
      GPIO_Led.list += adcNames[i];
      if (i < numAdc - 1) {
        GPIO_Led.list += ",";
      }
    }

 // Задаем выбранный элемент (по значению GPIO)
    for (int i = 0; i < numPins; i++) {
        if (pinValues[i] == config["GPIO_Gates_1"].toInt()) {
            GPIO_Gates_1.selected = i;
            break;
        }
    }
    for (int i = 0; i < numPins; i++) {
        if (pinValues[i] == config["GPIO_Gates_3"].toInt()) {
            GPIO_Gates_3.selected = i;
            break;
        }
    }
    for (int i = 0; i < numPins; i++) {
        if (adcValues[i] == config["GPIO_Led"].toInt()) {
            GPIO_Led.selected = i;
            break;
        }
    }

  bot.attachUpdate(updateh);   // подключить обработчик обновлений
  Serial.print("Бот токен: "); Serial.println(config["botToken"]);
  bot.setToken(F("5313332238:AAH_y7CKD9rovDF71Wmjke0K1uB58nAx064")); // установить токен
  //bot.setToken(F(BOT_TOKEN));  // установить токен
  //bot.setPollMode(fb::Poll::Long, 20000);
  Serial.print("CHAT_ID: ");
  Serial.println(config["CHAT_ID"]);
  // поприветствуем админа
  Serial.print("sendMessage Hello! - ");
  Serial.println(config["CHAT_ID"]);
  bot.setPollMode(fb::Poll::Long, 20000);
  bot.sendMessage(fb::Message("Hello!", CHAT_ID));
}

void loop() {
    static uint32_t tmr;
    ui.tick();
    bot.setToken(config["botToken"]);  // установить токен
    bot.tick();
    if (bot.tick()) Serial.println("bot.tick()");
    if (config.tick()) {
      Serial.println("Updated config");
      isSave = true;
    }
    
    if (millis() - tmr >= 1000) {
      tmr = millis();
        if (WiFi.status() == WL_CONNECTED) {
            //lbl.text = "Подключено к " + String(WiFi.SSID())+ " (" + String(WiFi.RSSI()) + ") " + "\r\n(IP: " + WiFi.localIP().toString() + ")";
            update = true;
            isWiFi = true;
        } else {
           //lbl.text = "WiFi не подключен.";
           update = false;
           isWiFi = false;
        }
    }
  updateGates();// Проверять состояние 
  if (isOpenGate1 || isOpenGate3) {
    int sensorValue = analogRead(config["GPIO_Led"].toInt()); // когда нажимаем какую либо кнопку, начинаем считывать значения с пина
    //Serial.print("sensorValue: ");
    //Serial.println(sensorValue);
  }
}

void formatLittleFS() {
    Serial.println("Форматируем LittleFS...");
    if (LittleFS.format()) {
        Serial.println("LittleFS успешно отформатирована.");
        Serial.println("Устанавливаем значения по умолчанию.");
        defaultConfig();
    } else {
        Serial.println("Ошибка форматирования LittleFS!");
    }
}

void defaultConfig() {
  #ifdef ESP8266
    config["GPIO_Gates_1"] = "16";
    config["GPIO_Gates_3"] = "5";
  #endif
  #ifdef ESP32
    config["GPIO_Gates_1"] = "2";
    config["GPIO_Gates_3"] = "18";  
  #endif
    config["GPIO_Led"] = "0";
    config["timeDownBtn"] = "1000";
    config["openThreshold"] = "3";
    config["ssid"] = "";      // Пустой SSID
    config["password"] = ""; // Пустой пароль
    config["botToken"] = BOT_TOKEN; //
    config["CHAT_ID"] = CHAT_ID; //

    config.update();
}


void printConfig() {
    Serial.print("GPIO_Gates_1: ");
    Serial.println(config["GPIO_Gates_1"]);
    Serial.print("GPIO_Gates_3: ");
    Serial.println(config["GPIO_Gates_3"]);
    Serial.print("timeDownBtn: ");
    Serial.println(config["timeDownBtn"]);
    Serial.print("GPIO_Led: ");
    Serial.println(config["GPIO_Led"]);
    Serial.print("timeDownBtn: ");
    Serial.println(config["openThreshold"]);
    Serial.print("ssid: ");
    Serial.println(config["ssid"]);
    Serial.print("password: ");
    Serial.println(config["password"]);
    Serial.print("openThreshold: ");
    Serial.println(config["openThreshold"]);
    Serial.print("botToken: ");
    Serial.println(config["botToken"]);
    Serial.print("CHAT_ID: ");
    Serial.println(config["CHAT_ID"]);
}


void openGate1() {
  startTime = millis();
  isOpenGate1 = true;
  digitalWrite(config["GPIO_Gates_1"], HIGH);
  Serial.print("Нажимаем пин ");
  Serial.println(config["GPIO_Gates_1"]);
}

void openGate3() {
  startTime = millis();
  isOpenGate3 = true;
  digitalWrite(config["GPIO_Gates_3"], HIGH);
  Serial.print("Нажимаем пин ");
  Serial.println(config["GPIO_Gates_3"]);
}

void updateGates() {
  if (isOpenGate1) {
    if (millis() - startTime >= config["timeDownBtn"].toInt()) {
      digitalWrite(config["GPIO_Gates_1"], LOW);
      Serial.print("Время удержания: ");
      Serial.println(millis() - startTime);
      Serial.print("Отпускаем пин ");
      Serial.println(config["GPIO_Gates_1"]);
      isOpenGate1 = false;
    }
  }
  if (isOpenGate3) {
    if (millis() - startTime >= config["timeDownBtn"].toInt()) {
      digitalWrite(config["GPIO_Gates_3"], LOW);
      Serial.print("Время удержания: ");
      Serial.println(millis() - startTime);
      Serial.print("Отпускаем пин ");
      Serial.println(config["GPIO_Gates_3"]);
      isOpenGate1 = false;
    }
  }
}

