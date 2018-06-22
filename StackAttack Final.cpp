#include "mbed.h"
#include "stdio.h"
#include "string"
#include "vector"

#include "SPI_TFT_ILI9341.h"
#include "GraphicsDisplay.h"

#include "Arial12x12.h"
#include "Arial24x23.h"
#include "Arial28x28.h"
#include "font_big.h"

#include "Sprites.h"

// Konstante: Velicina kutije, refresh rate, padding sa lijeva i sa vrha
#define BoxSize 30
#define FPS 24
#define LPAD 10

using std::vector;

//                      MOSI, MISO,  SCK,   CS, RESET,   DC, NAME
SPI_TFT_ILI9341 display(PTD2, PTD3, PTD1, PTE3, PTA20, PTD5, "display");
AnalogIn Jx(PTB1);
AnalogIn Jy(PTB2);
InterruptIn SW(PTE5);
Timer deb;

vector<vector<bool> > mapa;

double OffsetX, OffsetY;
bool paused(false);
bool gameover(false);



class Claw {
    int position, target;
    bool hasBox;
    double moveTime; // Vrijeme neophodno da se ruka pomjeri sa jedne lokacije na drugu, u milisekundama
public:
    Claw() : position(0), target(rand() % 10), hasBox(true), moveTime(1000) {}

    void Accelerate() {
        if(moveTime != 500) moveTime -= 100;
    } // Treba se pozivati kad built in timer izracuna neko vrijeme

    void GiveBox() {
        if (position == 0 && !hasBox) hasBox = true;
    }
    /*
    void MoveClaw(bool desno) {
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
            DrawClaw(position); //Izbaci ovo kad testiras komplikovaniju verziju
        }
    }*/

    void MoveClaw() {
        if (target > position)
            position++;
        else if (target < position)
            position--;
        else if (target == position){
            BaciBox();
            target = 0; // Pomakni claw na pocetak
        }
    }

    void BaciBox() {
        hasBox = false;
        mapa[position][5] = true; //Dodaj kutiju u
    }

    void DrawClaw() {
        //                         x0         y0                sirina   visina   *bitmap
        if(!hasBox) display.Bitmap(7*BoxSize, position*BoxSize, BoxSize, BoxSize, claw);
        else display.Bitmap(7*BoxSize, position*BoxSize, BoxSize, BoxSize, clawBox);
    }

    void TickerFja(){
        MoveClaw();
        DrawClaw();
    }
};

void gameOver();

class Player{
    short int x, y;
public:
    Player() : x(5),y(0){
        drawPlayer(x,y);
    }

    void PomjeriDesno(){
        if(x < 9){ //Ako moze jos ici desno
            if (mapa[x+1][y] == 0){ //Ako desno nije kutija
                x++;
            }
            else{ // Ako jeste kutija
                if (x+1 < 9){ //Ako nije kutija na kraju
                    if (mapa[x+2][y] == 0){ // Ako ima mjesta za pomjeranje kutije
                        x++; //Update lokacije
                        mapa[x][y] = 0;    //Brisi kutiju
                        mapa[x+1][y] = 1; //Pomjeri kutiju na novu lokaciju
                    }
                }
            }
            drawPlayer(x,y); // Nacrtaj ga na novoj lokaciji
        }
    }

    void PomjeriLijevo(){
        if(x > 0){ //Ako moze jos ici desno
            if (mapa[x-1][y] == 0){ //Ako desno nije kutija
                x--;
            }
            else{ // Ako jeste kutija
                if (x-1 > 0){ //Ako nije kutija na pocetku
                    if (mapa[x-2][y] == 0){ // Ako ima mjesta za pomjeranje kutije
                        x--; //Update lokacije
                        mapa[x][y] = 0;    //Brisi kutiju
                        mapa[x-1][y] = 1; //Pomjeri kutiju na novu lokaciju
                    }
                }
            }
            drawPlayer(x,y);
        }
    }

    void PomjeriGore(){
        if(y < 6) y++;
    }

    void PomjeriGoreDesno(){
        if (x < 9 && y < 6){ //Ima prostora za skok
            if (mapa[x+1][y+1] == 0){ // Pomjeri igraca ako nema kutije
                x++;
                y++;
            }
            else { // Ako ima kutije, pomakni kutiju
                if (x < 8){ // Ako nije boundary
                    if (mapa[x+2][y+1] == 0) {// Ako se moze pomaci desno
                        x++;
                        y++;
                        mapa[x][y] = 0; // Nema tu kutije
                        mapa[x+1][y] == 1; // Sad je pomaknuta desno
                    }
                }
            }
        }
    }

    void PomjeriGoreLijevo(){
        if (x > 0 && y < 6){ //Ima prostora za skok
            if (mapa[x-1][y+1] == 0){ // Pomjeri igraca ako nema kutije
                x--;
                y++;
            }
            else { // Ako ima kutije, pomakni kutiju
                if (x - 1 > 0){ // Ako nije boundary
                    if (mapa[x-2][y+1] == 0) {// Ako se moze pomaci desno
                        x--;
                        y++;
                        mapa[x][y] = 0; // Nema tu kutije
                        mapa[x-1][y] == 1; // Sad je pomaknuta desno
                    }
                }
            }
        }
    }

    void DaLiJeZiv(){
        if (mapa[x][y+1]) gameOver();
    }

    int GetX(){ return x; }
    void SetX(int x){ x = x; }
    void SetY(int y){ y = y; }

    void drawPlayer(int row, int column){
        //             x0           y0              širina   visina     *bitmap
        display.Bitmap(row*BoxSize, column*BoxSize, BoxSize, 2*BoxSize, charIdle);
    }
};

class Grid{
    Player p;
    int score;
public:
    Grid(){
        setupMap();
    }

    void setupMap(){
        p.SetX(5);
        p.SetY(0);
        p.drawPlayer(5, 0);
        score = 0;
        mapa.resize(0);
        vector<bool> novi(10, false);
        for (int i(0); i < 6; i++) mapa.push_back(novi);

        // Advanced verzija
        short int prvaKutija(rand() % 10), drugaKutija(rand() % 10), trecaKutija(rand() % 10), playerPozicija(rand()%10);
        while (prvaKutija == p.GetX()) drugaKutija = rand() % 10;
        while (drugaKutija == p.GetX() || drugaKutija == prvaKutija) trecaKutija = rand() % 10;
        while (trecaKutija == prvaKutija || trecaKutija == drugaKutija || trecaKutija == p.GetX()) trecaKutija = rand() % 10;
        mapa[0][prvaKutija] = mapa[0][drugaKutija] = mapa[0][trecaKutija] = true;

        //mapa[0][2] = mapa[0][4] = mapa[0][6] = true;
    }

    void drawMap(){
        display.cls();
        drawBorders();
        displayScore();
        // Crtanje mape ovisno o matrici prisustva
        for(int i(0); i<6; i++){
            for(int j(0); j<10; j++){
                if(mapa[i][j]) drawBox(i, j);
            }
        }
    }

    void refreshMapa(){ //Kad padaju kutije, treba se pozivati svaku sekundu
        for(int i(1); i<6; i++){
            for(int j(0); j<10; j++){
                if (mapa[i][j] == 1 && mapa[i-1][j] == 0){
                    mapa[i][j] == 0;
                    mapa[i-1][j] == 1;
                }
            }
        }
    }

    void calculatePoints(){
        bool rowFull(true);
        // Vidi kakvo je stanje
        for (int i(0); i < 9; i++){
            if (mapa[0][i] == 0) rowFull = false;
        }
        // Ako pun brisi sve
        if (rowFull){
            for (int i(0); i < 9; i++) mapa[0][i] = 0;
            score += 100;
            displayScore();
        }
    }

    void drawBox(int row, int column){
        display.rect(row*BoxSize, column*BoxSize + LPAD, row*BoxSize + BoxSize, column*BoxSize + BoxSize + LPAD, Black);
        display.line(row*BoxSize, column*BoxSize + LPAD, row*BoxSize + BoxSize, column*BoxSize + BoxSize + LPAD, Black);
        display.line(row*BoxSize + BoxSize, column*BoxSize + LPAD, row*BoxSize, column*BoxSize + BoxSize + LPAD, Black);
    }

    // Nacrtaj granice igre
    void drawBorders(){
        display.line(LPAD, 0, LPAD, 210, Red);      // Lijeva granica
        display.line(310, 0, 310, 210, Red);        // Desna granica
        display.line(LPAD, 210, 310, 210, Red);     // Donja granica
    }

    void displayScore(){
        display.locate(166, 219);
        display.printf("SCORE:%6d", score);
    }
};

Grid g;

void pause(){
    // Resetuje igru ako je trenutno kraj
    if(gameover){
        gameover = false;
        display.cls();
        g.setupMap();
        return;
    }

    if(deb.read_ms()>1000){
        // Toggle trenutno stanje pauze
        paused = !paused;

        if(paused){
            display.locate(LPAD, 219);
            display.printf("PAUZA");
            while(paused);
        } else {
            // Obriši tekst za pauzu, nastavi igru
            display.fillrect(LPAD, 219, 70, 231, White);
        }
    }
    deb.reset();
}

// WORK IN PROGRESS
void gameOver(){
    gameover = true;
    display.cls();

    display.locate(29, 100);
    display.set_font((unsigned char*) Arial28x28);
    display.printf("GAME OVER");
    display.set_font((unsigned char*) Arial12x12);
    display.locate(138, 130);
    display.printf("SCORE:%6d");

    while(1){
        if(gameover) wait(0.1);
        else break;
    }
}

void readJoystick(){
        int x = Jx*3.3 - 1.4;  // (-1) = Lijevo, 0 = Ništa, 1 = Desno
        int y = Jy*3.3 - 2.4;  // y = 1 ako se ide gore

        if(x<0){
            if(y) g.p.PomjeriGoreLijevo();
            else g.p.PomjeriLijevo();
        } else if(x>0){
            if(y) g.p.PomjeriGoreDesno();
            else g.p.PomjeriDesno();
        } else if(y) g.p.PomjeriGore();
}

// Zapis ofseta na osama u varijable. Ofsete treba sabrati sa očitanom vrijednosti.
void calibrateJoystick(){
    OffsetX = Jx*3.3 - 1.65;
    OffsetY = Jy*3.3 - 1.65;
}

// -------------------------------------------------------------------------- //

int main() {
    display.claim(stdout);
    display.set_orientation(1);
    display.background(White);
    display.foreground(Black);
    display.cls();
    display.set_font((unsigned char*) Arial12x12);

    // Dugme na džojstiku za pauzu
    //deb.start();
    //SW.mode(PullUp);
    //SW.rise(&pause);
    //calibrateJoystick();

    //Ticker koji pomjera i ispisuje claw
    Ticker clawTicker;
    Claw claw;
    clawTicker.attach(&claw, &Claw::TickerFja, 0.4);

    //Ticker za playera
    Ticker playerTicker;
    Player player;
    playerTicker.attach(&player, &Player::drawPlayer, 0.1);

    //Ticker za grid
    Ticker gridTicker;
    gridTicker.attach(&g, &Grid::drawMap, 0.4);

    //interrupt za kontroler

    while(1) {

    }
}
