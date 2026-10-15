#include "StyleManager.h"

QString StyleManager::applicationStyleSheet() {
    return R"(
        QMainWindow, QWidget#mainMenu {
            background-color: #20382f;
        }
        QLabel#eyebrow {
            color: #e1b86f;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#menuTitle {
            color: #f4f2e8;
            font-size: 36px;
            font-weight: 700;
        }
        QLabel#menuSubtitle {
            color: #c2d0c5;
            font-size: 15px;
        }
        QPushButton#menuButton {
            color: #20382f;
            background-color: #e6eadb;
            border: 2px solid #e6eadb;
            border-radius: 6px;
            padding: 14px 18px;
            text-align: left;
            font-size: 16px;
            font-weight: 600;
        }
        QPushButton#menuButton:hover {
            background-color: #f0c77d;
            border-color: #f0c77d;
        }
        QPushButton#menuButton:pressed {
            background-color: #dba958;
            border-color: #dba958;
        }
    )";
}