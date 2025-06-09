#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// constantes pour la configuration du jeu
#define LARGEUR_PLATEAU 80   // largeur totale du plateau de jeu
#define HAUTEUR_PLATEAU 40   // hauteur totale du plateau de jeu
#define TAILLE_PAVE 5        // taille des pavés (obstacles)
#define NB_PAVES 4           // nombre de pavés à placer sur le plateau
#define POSITION_INIT_X 40   // position initiale en x du serpent
#define POSITION_INIT_Y 20   // position initiale en y du serpent
#define DIRECTION_HAUT 'z'   // touche pour aller vers le haut
#define DIRECTION_BAS 's'    // touche pour aller vers le bas
#define DIRECTION_GAUCHE 'q' // touche pour aller vers la gauche
#define DIRECTION_DROITE 'd' // touche pour aller vers la droite
#define TOUCHE_QUITTER 'a'   // touche pour quitter le jeu
#define DELAI_BASE 100000    // délai entre chaque mouvement (en microsecondes)
#define BORDURE '#'          // caractère utilisé pour dessiner les bordures
#define FOND ' '             // caractère utilisé pour dessiner le fond
#define POMME '6'            // caractère représentant une pomme
#define CORPS 'X'            // caractère représentant le corps du serpent
#define TETE 'O'             // caractère représentant la tête du serpent

// tableau global représentant le plateau
char plateau[HAUTEUR_PLATEAU][LARGEUR_PLATEAU];

// variable globale pour la taille actuelle du serpent
int tailleSerpent = 10;

// prototypes des fonctions
int kbhit(); // vérifie si une touche a été pressée
void progresser(int lesX[], int lesY[], char direction, bool *statut, bool *aMangePomme); // gère le déplacement du serpent
void disableEcho(); // désactive l'écho du terminal
void enableEcho();  // réactive l'écho du terminal
void initPlateau(); // initialise le plateau avec les bordures et les pavés
void dessinerPlateau(); // affiche le plateau dans le terminal
void dessinerSerpent(int lesX[], int lesY[], int tailleSerpent); // dessine le serpent sur le plateau
void effacerSerpent(int lesX[], int lesY[], int tailleSerpent);  // efface le serpent du plateau
void ajouterPomme(); // place une pomme aléatoire sur le plateau

int main() {
    // coordonnées du serpent
    int lesX[tailleSerpent + 10], lesY[tailleSerpent + 10];

    // direction initiale du serpent
    char direction = DIRECTION_DROITE;

    // statut du jeu (true = en cours, false = terminé)
    bool statut = true;

    // temps de pause entre les mouvements du serpent
    int delai = DELAI_BASE;

    // initialisation des coordonnées du serpent
    for (int i = 0; i < tailleSerpent; i++) {
        lesX[i] = POSITION_INIT_X - (tailleSerpent - 1 - i); // corps initial s'étend vers la gauche
        lesY[i] = POSITION_INIT_Y; // tous les segments sont alignés sur la même ligne
    }

    disableEcho();    // désactivation de l'écho pour éviter l'affichage des touches
    initPlateau();    // initialisation du plateau
    ajouterPomme();   // placement de la première pomme

    // boucle principale du jeu
    while (statut) {
        effacerSerpent(lesX, lesY, tailleSerpent); // efface les anciennes positions du serpent

        // vérifie si une touche a été pressée
        if (kbhit()) {
            char touche = getchar();
            if (touche == TOUCHE_QUITTER) {
                statut = false; // quitte le jeu si la touche 'a' est pressée
            } else if ((touche == DIRECTION_DROITE && direction != DIRECTION_GAUCHE) ||
                       (touche == DIRECTION_GAUCHE && direction != DIRECTION_DROITE) ||
                       (touche == DIRECTION_HAUT && direction != DIRECTION_BAS) ||
                       (touche == DIRECTION_BAS && direction != DIRECTION_HAUT)) {
                direction = touche; // change la direction si elle est valide
            }
        }

        bool aMangePomme = false; // indique si le serpent a mangé une pomme

        // met à jour les positions du serpent et vérifie les collisions
        progresser(lesX, lesY, direction, &statut, &aMangePomme);

        // redessine le serpent après avoir vérifié les collisions
        dessinerSerpent(lesX, lesY, tailleSerpent);
        dessinerPlateau(); // affiche le plateau mis à jour

        if (aMangePomme) {
            ajouterPomme(); // ajoute une nouvelle pomme si le serpent en a mangé une
        }

        usleep(delai); // pause entre chaque mouvement
    }

    enableEcho(); // réactivation de l'écho du terminal
    printf("jeu terminé !\n");
    return EXIT_SUCCESS; // fin du programme
}

// fonction pour vérifier si une touche a été pressée
int kbhit() {
    int unCaractere = 0;
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar(); 

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        unCaractere = 1;
    }
    return unCaractere; 
}

// fonction pour désactiver l'écho du terminal (les touches tapées ne s'affichent pas)
void disableEcho() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty); 
    tty.c_lflag &= ~ECHO;         
    tcsetattr(STDIN_FILENO, TCSANOW, &tty); 
}

// fonction pour réactiver l'écho du terminal (les touches tapées s'affichent à nouveau)
void enableEcho() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;         
    tcsetattr(STDIN_FILENO, TCSANOW, &tty); 
}

// fonction pour initialiser le plateau de jeu
void initPlateau() {
    for (int y = 0; y < HAUTEUR_PLATEAU; y++) {
        for (int x = 0; x < LARGEUR_PLATEAU; x++) {
            // place des bordures en haut et en bas
            if (y == 0 || y == HAUTEUR_PLATEAU - 1) {
                plateau[y][x] = (x == LARGEUR_PLATEAU / 2) ? FOND : BORDURE;
            } 
            // place des bordures à gauche et à droite
            else if (x == 0 || x == LARGEUR_PLATEAU - 1) {
                plateau[y][x] = (y == HAUTEUR_PLATEAU / 2) ? FOND : BORDURE;
            } 
            // le reste est du fond
            else {
                plateau[y][x] = FOND;
            }
        }
    }

    srand(time(NULL)); // initialise le générateur aléatoire

    // ajoute des pavés aléatoires
    for (int p = 0; p < NB_PAVES; p++) {
        int px, py;
        do {
            px = rand() % (LARGEUR_PLATEAU - TAILLE_PAVE - 2) + 1;
            py = rand() % (HAUTEUR_PLATEAU - TAILLE_PAVE - 2) + 1;
        } while (plateau[py][px] != FOND); // vérifie que la zone est libre

        // remplit un pavé de la taille définie
        for (int dy = 0; dy < TAILLE_PAVE; dy++) {
            for (int dx = 0; dx < TAILLE_PAVE; dx++) {
                plateau[py + dy][px + dx] = BORDURE;
            }
        }
    }
}


// fonction pour afficher le plateau dans le terminal
void dessinerPlateau() {
    system("clear"); // efface le terminal avant d'afficher
    for (int y = 0; y < HAUTEUR_PLATEAU; y++) {
        for (int x = 0; x < LARGEUR_PLATEAU; x++) {
            putchar(plateau[y][x]); // affiche chaque caractère du plateau
        }
        putchar('\n'); // ajoute une nouvelle ligne à la fin de chaque rangée
    }
}

// fonction pour gérer le déplacement et les collisions du serpent
void progresser(int lesX[], int lesY[], char direction, bool *statut, bool *aMangePomme) {
    *aMangePomme = false; // réinitialise le statut de la pomme

    // calcule la prochaine position de la tête
    int nextX = lesX[tailleSerpent - 1];
    int nextY = lesY[tailleSerpent - 1];

    if (direction == DIRECTION_DROITE) nextX++;
    else if (direction == DIRECTION_GAUCHE) nextX--;
    else if (direction == DIRECTION_HAUT) nextY--;
    else if (direction == DIRECTION_BAS) nextY++;

    // gestion des portails
    if (nextX == 0 && lesY[tailleSerpent - 1] == HAUTEUR_PLATEAU / 2) {
        nextX = LARGEUR_PLATEAU - 2; // portail gauche-droite
    } else if (nextX == LARGEUR_PLATEAU - 1 && lesY[tailleSerpent - 1] == HAUTEUR_PLATEAU / 2) {
        nextX = 1; // portail droite-gauche
    } else if (nextY == 0 && lesX[tailleSerpent - 1] == LARGEUR_PLATEAU / 2) {
        nextY = HAUTEUR_PLATEAU - 2; // portail haut-bas
    } else if (nextY == HAUTEUR_PLATEAU - 1 && lesX[tailleSerpent - 1] == LARGEUR_PLATEAU / 2) {
        nextY = 1; // portail bas-haut
    }
    // collision avec une bordure
    else if (plateau[nextY][nextX] == BORDURE) {
        *statut = false; // le serpent meurt
        return;
    }

    // collision avec le corps
    for (int i = 0; i < tailleSerpent; i++) {
        if (lesX[i] == nextX && lesY[i] == nextY) {
            *statut = false; // le serpent meurt
            return;
        }
    }

    // collision avec une pomme
    if (plateau[nextY][nextX] == POMME) {
        *aMangePomme = true; // indique que la pomme a été mangée
    }

    // décalage des segments du corps
    for (int i = 0; i < tailleSerpent - 1; i++) {
        lesX[i] = lesX[i + 1];
        lesY[i] = lesY[i + 1];
    }

    // met à jour la tête
    lesX[tailleSerpent - 1] = nextX;
    lesY[tailleSerpent - 1] = nextY;

    // ajoute un segment si une pomme a été mangée
    if (*aMangePomme) {
        lesX[tailleSerpent] = lesX[tailleSerpent - 1];
        lesY[tailleSerpent] = lesY[tailleSerpent - 1];
        tailleSerpent++;
    }
}

// fonction pour dessiner le serpent sur le plateau
void dessinerSerpent(int lesX[], int lesY[], int tailleSerpent) {
    for (int i = 0; i < tailleSerpent - 1; i++) {
        plateau[lesY[i]][lesX[i]] = CORPS; // dessine le corps du serpent
    }
    plateau[lesY[tailleSerpent - 1]][lesX[tailleSerpent - 1]] = TETE; // dessine la tête du serpent
}

// fonction pour effacer le serpent du plateau
void effacerSerpent(int lesX[], int lesY[], int tailleSerpent) {
    for (int i = 0; i < tailleSerpent; i++) {
        plateau[lesY[i]][lesX[i]] = FOND; // remplace chaque segment par le fond
    }
}

// fonction pour ajouter une pomme aléatoire sur le plateau
void ajouterPomme() {
    int x, y;
    do {
        x = rand() % (LARGEUR_PLATEAU - 2) + 1; // génère une position x aléatoire
        y = rand() % (HAUTEUR_PLATEAU - 2) + 1; // génère une position y aléatoire
    } while (plateau[y][x] != FOND); // vérifie que la position est libre

    plateau[y][x] = POMME; // place une pomme sur le plateau
}
