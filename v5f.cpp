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
SPI_TFT_ILI9341 display(PTD2, PTD3, PTD1, PTE2, PTA20, PTD5, "display");
AnalogIn Jx(PTB1);
AnalogIn Jy(PTB2);
//InterruptIn SW(PTE5);
Timer deb;

vector<vector<bool> > mapa;

double OffsetX, OffsetY;
bool paused(false);
bool gameover(false);
bool toggleDraw(true);



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
        if (position == 0 && !hasBox){
            hasBox = true;
            target = rand()%10;
        }
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
        GiveBox();
        if (target > position) position++;
        else if (target < position) position--;
        else if (target == position) BaciBox();
    }

    void BaciBox() {
        hasBox = false;
        mapa[5][position] = true; // Dodaj kutiju u
        target = 0; // Pokupi novu na pocetku
    }

    void DrawClaw() {
        //                         x0         y0               sirina   visina   *bitmap
        if(!hasBox) display.Bitmap(position*BoxSize + LPAD, 0, BoxSize, BoxSize, claw);
        else display.Bitmap(position*BoxSize + LPAD, 0, BoxSize, BoxSize, clawBox);
    }

    void drawBlank(){
        display.fillrect(position*BoxSize + LPAD, 0, position*BoxSize + BoxSize + LPAD, BoxSize, White);
    }

    void TickerFja(){
        drawBlank();
        MoveClaw();
        DrawClaw();
    }
};

void gameOver();

class Player{
    short int x, y;
public:
    Player() : x(5),y(0){}

    void PomjeriDesno(){
        if(x < 9){ //Ako moze jos ici desno
            if (mapa[y+1][x] == 0){ //Ako desno nije kutija
                x++;
            }
            else{ // Ako jeste kutija
                if (x+1 < 9){ // Ako nije kutija na kraju
                    if (mapa[y+2][x] == 0){ // Ako ima mjesta za pomjeranje kutije
                        x++; //Update lokacije
                        mapa[y][x] = 0;    //Brisi kutiju
                        mapa[y+1][x] = 1; //Pomjeri kutiju na novu lokaciju
                    }
                }
            }
        }
    }

    void PomjeriLijevo(){
        if(x > 0){ //Ako moze jos ici desno
            if (mapa[y-1][x] == 0){ // Ako lijevo nije kutija
                x--;
            }
            else{ // Ako jeste kutija
                if (x-1 > 0){ //Ako nije kutija na kraju
                    if (mapa[y-2][x] == 0){ // Ako ima mjesta za pomjeranje kutije
                        x--; //Update lokacije
                        mapa[y][x] = 0;    //Brisi kutiju
                        mapa[y-1][x] = 1; //Pomjeri kutiju na novu lokaciju
                    }
                }
            }
        }
    }

    void PomjeriGore(){
        if(y < 6) y++;
    }

    void PomjeriGoreDesno(){
        if (x < 9 && y < 6){ //Ima prostora za skok
            if (mapa[y+1][x+1] == 0){ // Pomjeri igraca ako nema kutije
                x++;
                y++;
            }
            else { // Ako ima kutije, pomakni kutiju
                if (x < 8){ // Ako nije boundary
                    if (mapa[y+2][x+1] == 0) {// Ako se moze pomaci desno
                        x++;
                        y++;
                        mapa[y][x] = 0; // Nema tu kutije
                        mapa[y+1][x] == 1; // Sad je pomaknuta desno
                    }
                }
            }
        }
    }

    void PomjeriGoreLijevo(){
        if (x > 0 && y < 6){ //Ima prostora za skok
            if (mapa[y-1][x+1] == 0){ // Pomjeri igraca ako nema kutije
                x--;
                y++;
            }
            else { // Ako ima kutije, pomakni kutiju
                if (x - 1 > 0){ // Ako nije boundary
                    if (mapa[y-2][x+1] == 0) {// Ako se moze pomaci desno
                        x--;
                        y++;
                        mapa[y][x] = 0; // Nema tu kutije
                        mapa[y-1][x] == 1; // Sad je pomaknuta desno
                    }
                }
            }
        }
    }

    void DaLiJeZiv(){
        if (mapa[y+1][x]) gameOver();
    }

    int GetX(){ return x; }
    void SetX(int x){ x = x; }
    void SetY(int y){ y = y; }

    void drawPlayer(){
        //     x0           y0              喨rina   visina     *bitmap
        display.Bitmap(x*BoxSize + LPAD, (5-y)*BoxSize, BoxSize, 2*BoxSize, charIdle);
    }
};

class Grid{
    Player p;
    int score;
public:
    Grid(){
        setupMap();
    }

    Player& Igrac(){ return p; }

    void setupMap(){
        p.SetX(5);
        p.SetY(0);
        score = 0;
        mapa.resize(0);
        vector<bool> novi(10, false);
        for (int i(0); i < 6; i++) mapa.push_back(novi);

        mapa[0][0] = mapa[0][1] = mapa[0][7] = true;
    }

    int GetScore(){ return score; }

    void drawMap(){
        drawBorders();
        displayScore();
        // Crtanje mape ovisno o matrici prisustva
        for(int i(0); i<6; i++){
            for(int j(0); j<10; j++){
                if(mapa[i][j]) drawBox(j, 6-i);
                else drawBlank(j, 6-i);
            }
        }
        calculatePoints();
        if(toggleDraw = !toggleDraw) refreshMapa();
        p.DaLiJeZiv();
    }

    void refreshMapa(){ //Kad padaju kutije, treba se pozivati svaku sekundu
        for(int i(1); i<6; i++){
            for(int j(0); j<10; j++){
                if (mapa[i][j] && !mapa[i-1][j]){
                    mapa[i][j] = false;
                    mapa[i-1][j] = true;/*
                    if (mapa[1][4] == true && mapa[0][4] == false){
                        mapa[1][4] = false;
                        mapa[0][4] = true;
                    }*/
                }
            }
        }
    }

    void calculatePoints(){
        bool rowFull(true);
        // Vidi kakvo je stanje
        for (int i(0); i < 10; i++){
            if (mapa[0][i] == 0) rowFull = false;
        }
        // Ako pun brisi sve
        if (rowFull){
            for (int i(0); i < 10; i++) mapa[0][i] = 0;
            score += 100;
            displayScore();
        }
    }

    void drawBox(int row, int column){
        display.rect(row*BoxSize + LPAD, column*BoxSize, row*BoxSize + BoxSize + LPAD, column*BoxSize + BoxSize, Black);
        display.line(row*BoxSize + LPAD, column*BoxSize, row*BoxSize + BoxSize + LPAD, column*BoxSize + BoxSize, Black);
        display.line(row*BoxSize + BoxSize + LPAD, column*BoxSize, row*BoxSize + LPAD, column*BoxSize + BoxSize, Black);
    }

    void drawBlank(int row, int column){
        display.fillrect(row*BoxSize + LPAD + 1, column*BoxSize, row*BoxSize + BoxSize + LPAD, column*BoxSize + BoxSize - 1, White);
    }

    // Nacrtaj granice igre
    void drawBorders(){
        display.line(LPAD - 1, 0, LPAD - 1, 211, Red);      // Lijeva granica
        display.line(311, 0, 311, 210, Red);                // Desna granica
        display.line(LPAD - 1, 211, 311, 211, Red);         // Donja granica
    }

    void displayScore(){
        display.locate(166, 219);
        display.printf("SCORE:%6d", score);
    }
};

Grid g;
Ticker clawTicker;
Ticker playerTicker;
Ticker gridTicker;

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
    clawTicker.detach();
    playerTicker.detach();
    gridTicker.detach();
    gameover = true;
    display.cls();

    display.locate(58, 100);
    display.set_font((unsigned char*) Arial28x28);
    display.printf("GAME OVER");
    display.set_font((unsigned char*) Arial12x12);
    display.locate(100, 140);
    display.printf("SCORE:%6d", g.GetScore());
    display.locate(70, 180);
    display.printf("Press Joystick to restart...");

    while(1){
        if(gameover) wait(0.1);
        else break;
    }
}

void readJoystick(){
        // Debouncing džojstika
        if(deb.read_ms()<0.4) return;
        deb.reset();

        int x = Jx*3.3 - 1.4;  // (-1) = Lijevo, 0 = Ništa, 1 = Desno
        int y = Jy*3.3 - 2.4;  // y = 1 ako se ide gore

        if(x<0){
            if(y) g.Igrac().PomjeriGoreLijevo();
            else g.Igrac().PomjeriLijevo();
        } else if(x>0){
            if(y) g.Igrac().PomjeriGoreDesno();
            else g.Igrac().PomjeriDesno();
        } else if(y) g.Igrac().PomjeriGore();
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
    display.background(Orange);
    display.foreground(Black);
    display.cls();
    display.set_font((unsigned char*) Arial12x12);

    // Dugme na džojstiku za pauzu
    deb.start();
    //SW.mode(PullUp);
    //SW.rise(&pause);
    calibrateJoystick();


    //Ticker koji pomjera i ispisuje claw
    Claw claw;
    clawTicker.attach(&claw, &Claw::TickerFja, 0.5);

    //Ticker za playera
    Player player(g.Igrac());
    playerTicker.attach(&player, &Player::drawPlayer, 0.1);

    gridTicker.attach(&g, &Grid::drawMap, 0.4);
    player.PomjeriLijevo();


    while(1) {
        //readJoystick();
        wait(1);
    }
}
