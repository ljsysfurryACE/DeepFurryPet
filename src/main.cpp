#include <QApplication>
#include "petwindow.h"
#include "settingsmanager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DeepFurryPet");
    app.setOrganizationName("DeepFurry");
    app.setQuitOnLastWindowClosed(true);
    
    PetWindow pet;
    pet.show();
    
    return app.exec();
}
