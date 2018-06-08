#include "mbed.h"
#include "stdio.h"
#include "string"
#include <vector>
#include <ctime>

#include "SPI_TFT_ILI9341.h"
#include "GraphicsDisplay.h"

#include "Arial12x12.h"
#include "Arial24x23.h"
#include "Arial28x28.h"
#include "font_big.h"

#include "Sprites.h"

#define BoxSize 30
#define FPS 24

//                       MOSI,  MISO,   SCK,    CS, RESET,    DC,  NAME
SPI_TFT_ILI9341 display(PTC17, PTA16, PTA17, PTC16, PTA20, PTC13, "display");
AnalogIn Jx(PTB1);
AnalogIn Jy(PTB2);
InterruptIn SW(PTE5);
Timer deb;

using std::vector;
using Sprites;

// Vektor se koristi umjesto nizova jer je dosta memorijski efikasniji za bool
vector<vector<bool> > mapa;

double OffsetX, OffsetY;
bool paused(false);

//
void drawCharacter(int row, int column){
    Bitmap(row*)
}
Bitmap bmCharIdle(BoxSize, BoxSize*2, 20, charIdle);

void drawMap();
void drawBox(int row, int column);
void drawPlayer(int row, int column);
void pause();
void CalibrateJoystick(double &Ox, double $Oy);
void defaultMapa(vector<vector<bool>> &pocetnaMapa);


class Claw {
    int position;
    bool hasBox;
    double moveTime; //Vrijeme neophodno da se ruka pomjeri sa jedne lokacije na drugu, u milisekundama

public:
    Claw() : position(0), hasBox(true), moveTime(1000) {
        DrawClaw(0);
        MoveClaw(rand() % 10);
    }

    void Accelerate() {
        if (moveTime != 500) //Max ubrzanje 2x
            moveTime -= 100;
    } //Treba se pozivati kad built in timer izracuna neko vrijeme

    void GiveBox() {
        if (position == 0 && !hasBox) hasBox = true;
    }

    void MoveClaw(bool &desno) {
        if (position < 9 && desno) {
            for (double i(position); i < i+1; i+= 1/FPS){
                DrawClaw(i);
                wait(moveTime / FPS);
            }
            position++;
        }

        else if (position > 0 && !desno) {
            for (double i(position); i > i - 1; i -=  1 / FPS){
                DrawClaw(i);
                wait(moveTime / FPS);
            }
            position--;
        }
    }

    void MoveClaw(int &pozicija) {
        while (pozicija > position)
            MoveClaw(true);

        while (pozicija < position)
            MoveClaw(false);

        BaciBox();
        MoveClaw(0); //Vrati je na pocetak
    }

    void BaciBox() {
        hasBox = false;
        DrawClaw(position);
        drawBox(position, 7); // Nacrtaj box koji ce da pada, 7 je drop zone
        //Nije dovoljno, moramo nekako simulirati mehanizam padanja
    }

    void DrawClaw(int &pozicija) {
        if(!hasBox)
            //Nacrtaj bez boxa
        else
            //Nacrtaj sa boxom
    }
};

int main() {
    display.claim(stdout);
    // Željena orijentacija
    display.set_orientation(1);
    display.background(Orange);
    display.foreground(Black);
    display.cls();
    /* Fontovi koji se koriste: 12x12 za obični tekst i 28x28 za veliki tekst
    display.set_font((unsigned char*) Arial12x12);
    display.set_font((unsigned char*) Arial28x28);                            */

    // Kalibriraj džojstik i zapiši offsete
    CalibrateJoystick(OffsetX, OffsetY);

    for(int i(0); i<6; i++) mapa.push_back(vector<bool>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    //Dodaj tri kutije na ekran
    defaultMapa(mapa);

    display.set_font((unsigned char*) Arial12x12);
    deb.start();
    SW.mode(PullUp);
    SW.rise(&pause);

    while(1){
        drawMap();
        wait(0.75);
    }
}

void drawBox(int row, int column){
    display.rect(row*BoxSize, column*BoxSize, row*BoxSize + BoxSize, column*BoxSize + BoxSize, Black);
    display.line(row*BoxSize, column*BoxSize, row*BoxSize + BoxSize, column*BoxSize + BoxSize, Black);
    display.line(row*BoxSize + BoxSize, column*BoxSize, row*BoxSize, column*BoxSize + BoxSize, Black);
}

void drawMap(){
    display.cls();
    // Crtanje mape ovisno o matrici prisustva
    for(int i(0); i<6; i++){
        for(int j(0); j<10; j++){
            if(mapa[i][j]) drawBox(i, j);
        }
    }
    // Crtanje modela igrača jer je ekran obrisan

}

void defaultMapa(vector<vector<bool>> &pocetnaMapa) {
    /*
    // Advanced verzija
    short int prvaKutija(rand() % 10), drugaKutija(rand() % 10), trecaKutija(rand() % 10);
    while (prvaKutija == drugaKutija) drugaKutija = rand() % 10;
    while (prvaKutija == trecaKutija || drugaKutija == trecaKutija) trecaKutija = rand() % 10;
    pocetnaMapa[0][prvaKutija] = pocetnaMapa[0][drugaKutija] = pocetnaMapa[0][trecaKutija] = true;
    */
    pocetnaMapa[0][2] = pocetnaMapa[0][4] = pocetnaMapa[0][6] = true;
}

void pause(){
    // pozvati wait(1) koji se breaka kad se ponovo pritisne pause
    if(deb.read_ms()>200){
        if(!paused){
            display.set_font((unsigned char*) Arial28x28);
            display.locate(90, 0);
            display.printf("PAUZA");
            display.set_font((unsigned char*) Arial12x12);
        } else {
            // Obriši pauzirano stanje
            display.fillrect(90, 0, 230, 28, Orange);
        }
    }
    deb.reset();
}

// Zapis ofseta na osama u varijable. Ofsete treba oduzeti od očitane vrijednosti.
void CalibrateJoystick(double &Ox, double &Oy){
    Ox = Jx*3.3 - 1.65;
    Oy = Jy*3.3 - 1.65;
}

void drawPlayer(int row, int column){
    //     x0           y0              širina   visina     *bitmap
    Bitmap(row*BoxSize, column*BoxSize, BoxSize, 2*BoxSize, charIdle);
}


/*                           *** IDEA LIST ***

  * U programskoj logici predstaviti displej kao matricu, gdje svaki član
    matrice odgovara veličini jedne kutije. Na ovaj način se pojednostavi
    sami dizajn i problem prikaza kutija i igrača na displej.
    Ovo bi se moglo realizovati kao matrica booleana, tako da se što više
    uštedi na ograničenoj memoriji.
    Osim pojednostavljenog prikaza igrača i kutija, preko ove matrice također
    možemo veoma lagano realizovati object collision - sve što je potrebno je
    da provjerimo ako su susjedni članovi matrice označeni kao zauzeti ili ne.

  * Crtanje se može vršiti preko matrice pripadnosti, preko funkcije DrawBox
    za svako zauzeto mjesto

  * Dimenzije displeja su 240x320, što trebamo okrenuti tako da bude 320x240,
    jer nam je u interesu da imamo šire polje za slaganje kutija.
    Na osnovu ovih dimenzija, trebamo odabrati adekvante dimenzije za kutije.
    Radi grafičke prezentacije, možemo dimenzije smanjiti na 300x180, čime sa
    dimenzijama kutije 30x30 dobijamo polje 10x6, što se čini kao sasvim
    solidno igračko polje.
    "Odsječene" piksele možemo iskoristiti za prikaz poda, plafona, i strana
    skladišta u kojem se igra odvija.
    Na y-osi imamo 60 piksela viška. Gornjih 30 možemo iskoristiti za prikaz
    mašine koja donosi nove kutije, dok 30px odmah ispod možemo koristiti kao
    "buffer" zonu za te nove kutije.

  * Dugme koje je ugrađeno u mikrokontroler možemo iskoristiti za pauziranje.
    Ovo bi se moglo lagano realizovati preko InterruptIn i jedne logičke
    varijable koja označava stanje (Pauza-Igra), te samo postavimo wait(1) i
    if uslov koji izlazi iz petlje kada se dugme pritisne.
    Eventualno, možemo iskoristiti pritisak džojstika za pauziranje.

  * Resursi za generisanje zvukova za moguću muziku u pozadini:
    https://stackoverflow.com/questions/4060601/make-sounds-beep-with-c
    https://bisqwit.iki.fi/jutut/kuvat/programming_examples/midimake.cc

  * Input džojstika se treba očitavati preko InterruptIn, unutar kojeg se vrši
    provjera ako je dati pokret moguć (radi prepreka) i gdje se pokret obrađuje.

  * Pixel art se treba nacrtati pa konvertovati u hex fajl.
    Stranica za crtanje: https://www.pixilart.com/draw
    IMG2HEX: http://www.thaieasyelec.net/archives/Manual/IMG2HEX.rar

    Alternativno:
    https://os.mbed.com/users/dreschpe/code/SPI_TFT_ILI9341/wiki/Homepage#graphics

  * Animacije bi se trebale dodati za svaki objekt ovisno od smjera u kojem se
    kreće. Tako bi za recimo Claw trebali imati AnimLeft i AnimRight.


  *



                            *** PROBLEM LIST ***
  * P: Izbjeći crtanje kutije gdje se nalazi model igrača?
    R: Nacrtati kutiju i odmah precrtati igrača preko.

  * P: Riješiti gravitaciju (igrač je u skoku, treba pasti)
    R: Ako ispod igrača u matrici prisustva nema ništa, spustiti
    ga na pod tek na kraju faze za pokret, tako da može skočiti u stranu.

  * P: Animirati padajuće kutije, te kretanje igrača i pomjeranje kutija.


  *


  *


*/
