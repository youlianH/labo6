 /*
 * @file main.c  
 * @author 
 * @date   
 * @brief  
 *
 * @version 1.0
 * Environnement:
 *     Développement: MPLAB X IDE (version 5.05)
 *     Compilateur: XC8 (version 2.00)
 *     Matériel: Carte démo du Pickit3. PIC 18F45K20
  */

/****************** Liste des INCLUDES ****************************************/
#include <xc.h>
#include <stdbool.h>  // pour l'utilisation du type bool
#include <conio.h>
#include <string.h> //pour strcmp 
#include "Lcd4Lignes.h"
#include "serie.h" 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/********************** CONSTANTES *******************************************/
#define _XTAL_FREQ 1000000 //Constante utilisée par __delay_ms(x). Doit = fréq interne du uC
#define DELAI_TMR0 0x0BDC 
#define NB_LIGNE 4  //afficheur LCD 4x20
#define NB_COL 20
#define AXE_X 7  //canal analogique de l'axe x
#define AXE_Y 6
#define PORT_SW PORTBbits.RB1 //sw de la manette
#define TUILE 1 //caractère cgram d'une tuile
#define MINE 2 //caractère cgram d'une mine
/********************** PROTOTYPES *******************************************/
void initialisation(void);
void initTabVue(void);
void rempliMines(int nb);
void metToucheCombien(void);
char calculToucheCombien(int ligne, int colonne);
void deplace(char* x, char* y);
bool demine(char x, char y);
void enleveTuilesAutour(char x, char y);
bool gagne(int* pMines);
void afficheTabVue(void);
 

/****************** VARIABLES GLOBALES ****************************************/
char m_tabVue[NB_LIGNE][NB_COL+1]; //Tableau des caractères affichés au LCD
char m_tabMines[NB_LIGNE][NB_COL+1];//Tableau contenant les mines, les espaces et les chiffres

/*               ***** PROGRAMME PRINCPAL *****                             */
void main(void)
{
    initialisation();
    init_serie();
    lcd_init();
    initTabVue();
    lcd_curseurHome();
    
    int NBMines=12;
    char x =1;
    char y = 1;
    lcd_putMessage("LAB6 Youlian Houehounou");
    rempliMines(NBMines);
    metToucheCombien();
    afficheTabVue();
    
    while(1) //boucle principale
    {
       deplace(&x,&y);
        if (PORT_SW==true)
        {
            if((demine(x,y)==false)||(gagne(NBMines)==true))
            {
                afficheTabVue();
            }
            while (PORT_SW==true);
            initTabVue();
            rempliMines(NBMines);
            metToucheCombien();
            afficheTabVue();
        }
        __delay_ms(100);
        /*
        rempliMines(20);
        metToucheCombien();   
       afficheTabVue();
        */
    }
}

/**
 * @brief Fait l'initialisation des différents regesitres du PIC
 * @param Aucun
 * @return Aucun
 */
void initialisation(void)
{
    TRISD = 0; //Tout le port D en sortie

//    ANSELH = 0;  // RB0 à RB4 en mode digital. Sur 18F45K20 AN et PortB sont sur les memes broches
//    TRISB = 0xFF; //tout le port B en entree
  
    ANSEL = 0;  // PORTA en mode digital. Sur 18F45K20 AN et PortA sont sur les memes broches
    TRISA = 0; //tout le port A en sortie
     //Configuration du port analogique
    ANSELbits.ANS7 = 1;  //A7 en mode analogique
 
    ADCON0bits.ADON = 1; //Convertisseur AN à on
	ADCON1 = 0; //Vref+ = VDD et Vref- = VSS
 
    ADCON2bits.ADFM = 0; //Alignement à gauche des 10bits de la conversion (8 MSB dans ADRESH, 2 LSB à gauche dans ADRESL)
    ADCON2bits.ACQT = 0;//7; //20 TAD (on laisse le max de temps au Chold du convertisseur AN pour se charger)
    ADCON2bits.ADCS = 0;//6; //Fosc/64 (Fréquence pour la conversion la plus longue possible)
}
/*
 * @brief Lit le port analogique. 
 * @param Le no du port à lire
 * @return La valeur des 8 bits de poids forts du port analogique
 */
char getAnalog(char canal)
{ 
    ADCON0bits.CHS = canal;
    __delay_us(1);  
    ADCON0bits.GO_DONE = 1;  //lance une conversion
    while (ADCON0bits.GO_DONE == 1) //attend fin de la conversion
        ;
    return  ADRESH; //retourne seulement les 8 MSB. On laisse tomber les 2 LSB de ADRESL
}

/*
 * @brief Rempli le tableau m_tabVue avec le caractère spécial (définie en CGRAM
 *  du LCD) TUILE. Met un '\0' à la fin de chaque ligne pour faciliter affichage
 *  avec lcd_putMessage().
 * @param rien
 * @return rien
 */
void initTabVue(void)
{
    int i=0;
    int j=0;
    for ( i=0;i<NB_LIGNE;i++)
    {
        for (j=0;j<(NB_COL+1);j++)
        {
            if(i<20)
            {
                m_tabVue[i][j]= TUILE;
            }
            
            if (j==20)
            {
                m_tabVue[i][j]= '\0';
            }
        }
    }
    
}
 
/*
 * @brief Rempli le tableau m_tabMines d'un nombre (nb) de mines au hasard.
 *  Les cases vides contiendront le code ascii d'un espace et les cases avec
 *  mine contiendront le caractère MINE défini en CGRAM.
 * @param int nb, le nombre de mines à mettre dans le tableau 
 * @return rien
 */
void rempliMines(int nb)
{
    int i=0;
    int j=0;
    char x =0;
    char y =0;
    char nbMine =0;
    
    for (int i=0;i<NB_LIGNE;i++)//La boucle mets les valeurs des lignes su tableau tabLcd à false.
    {
        for (int j=0;j<NB_COL;j++)//La boucle met les valeurs des colonnes du tableau tablcd à false.
        {
            m_tabMines[i][j]=32;
        }
    }  
    
    while (nbMine!=nb)
    {
        x = rand()%20;
        y = rand()%4;
        if (m_tabMines[y][x]==32)
        {
            m_tabMines[y][x] = MINE;
            nbMine = nbMine + 1;
        }
    }
}
 
/*
 * @brief Rempli le tableau m_tabMines avec le nombre de mines que touche la case.
 * Si une case touche à 3 mines, alors la méthode place le code ascii de 3 dans
 * le tableau. Si la case ne touche à aucune mine, la méthode met le code
 * ascii d'un espace.
 * Cette méthode utilise calculToucheCombien(). 
 * @param rien
 * @return rien
 */
void metToucheCombien(void)
{
    int i=0;
    int j=0;
    int mine=0;
    
    for ( i=0;i<NB_LIGNE;i++)
    {
        for (j=0;j<NB_COL;j++)
        {
            if(m_tabMines[i][j]!= MINE)
            {
                mine = calculToucheCombien(i,j);
                if (mine==0)
                {
                    m_tabMines[i][j]=32;
                }
                if (mine>0)
                {
                    m_tabMines[i][j]= (mine+48);
                }
            }  
        }
    }
}

 
/*
 * @brief Calcul à combien de mines touche la case. Cette méthode est appelée par metToucheCombien()
 * @param int ligne, int colonne La position dans le tableau m_tabMines a vérifier
 * @return char nombre. Le nombre de mines touchées par la case
 */
char calculToucheCombien(int ligne, int colonne)
{
    int nb_mine =0;
    int i=0;
    int j=0;
    if ((ligne<3)&&(ligne>0)&&(colonne>0)&&(colonne<20))
    {
        for(i=-1;i<2;i++)
        {
            for(j=-1;j<=1;j++)
            {
                if (m_tabMines[ligne+(i)][colonne+(j)]== MINE)
                {
                    nb_mine++;
                } 
            }
        }
    }
    if((ligne==0)&&(colonne==0))
    {
        if(m_tabMines[ligne][colonne+1]==MINE)
        {
            nb_mine++;
        }
        for(j=0;j<=1;j++)
        {
            if(m_tabMines[ligne+1][colonne+(j)]==MINE)
            {
                nb_mine++;
            }
        }
    }
    if((ligne==0)&&(colonne<20)&&(colonne>0))
    {
        for(i=-1;i<=1;i++)
        {
            if(m_tabMines[ligne][colonne+(i)]==MINE)
            {
                nb_mine ++; 
            }
            i++;
        }
        for(i=1;i<=1;i++)
        {
            for(j=-1;j<=1;j++)
            {
                if(m_tabMines[ligne+(i)][colonne+(j)]==MINE)
                {
                    nb_mine ++; 
                }
            }
        }
    } 
    if((ligne==0)&&(colonne==20))
    {
        if(m_tabMines[ligne][colonne-1]==MINE)
        {
            nb_mine++;
        }
        for(i=-1;i<1;i++)
        {
            if(m_tabMines[ligne+1][colonne+i]==MINE)
            {
                nb_mine++;
            }
        }
    }    
    if((ligne==3)&&(colonne==0))
    {
        if(m_tabMines[ligne-1][colonne]==MINE)
        {
            nb_mine++;
        }
        for(i=-1;i<=0;i++)
        {
            if(m_tabMines[ligne+(i)][colonne+1]==MINE)
            {
                nb_mine++;
            }
        }
    }
    if((ligne==3)&&(colonne<20)&&(colonne>0))
    {
        for(j=-1;j<=1;j++)
        {
            if(m_tabMines[ligne-1][(colonne+j)]== MINE)
            {
                nb_mine++;
            }
        }
        for(i=-1;i<=1;i++)
        {
            if(m_tabMines[ligne][(colonne+i)]==MINE)
            {
                nb_mine++;
            }
            i++;
        }
    }
    if((ligne==3)&&(colonne==20))
    {
        for(i=-1;i<1;i++)
        {
            if(m_tabMines[ligne-1][colonne+i]== MINE)
            {
              nb_mine++;   
            }
        }      
        if(m_tabMines[ligne][colonne-1]==MINE)
        {
            nb_mine++;
        }   
    }
    return nb_mine;
}
/** 

 * @brief Si la manette est vers la droite ou la gauche, on déplace le curseur 
 * d'une position (gauche, droite, bas et haut)
 * @param char* x, char* y Les positions X et y  sur l'afficheur
 * @return rien
 */
void deplace(char* x, char* y)
{
    lcd_gotoXY( *x,*y);
    if(getAnalog(AXE_X)<110)//Lorsque l'analogue est dirigé vers la droite la position du cursseur augmente de 1.
    {
       (*x)= (*x) +1;
       if (*x > 20)//Lorsque le cursseur dépasse le 20e segment du LCD il réapparaît sur le 1er segment.
       {
           *x=1;
       }
    }
    else if(getAnalog(AXE_X)>200)//Lorsque l'analogue est dirigé vers la gauche la position du cursseur diminue de 1.
    {
       (*x)=(*x)-1;
       if((*x)<1)
       {
           (*x)=20;//Lorsque le cursseur dépasse le 1er segment du LCD il réapparaît sur le 20ème segment.
       }
    }
    if(getAnalog(AXE_Y)<110)
    {
        (*y)= (*y) -1;
        if (*y < 1)//Lorsque le cursseur dépasse le 1e segment du LCD il réapparaît sur le 4er segment.
        {
            *y=4;
        }
    }
    else if(getAnalog(AXE_X)>200)
    {
        (*y)=(*y)+1;
        if (*y > 4)//Lorsque le cursseur dépasse le 20e segment du LCD il réapparaît sur le 1er segment.
        {
            *y=1;
        }   
    }
    lcd_gotoXY(*x ,*y);
}
 
/*
 * @brief Dévoile une tuile (case) de m_tabVue. 
 * S'il y a une mine, retourne Faux. Sinon remplace la case et les cases autour
 * par ce qu'il y a derrière les tuiles (m_tabMines).
 * Utilise enleveTuileAutour().
 * @param char x, char y Les positions X et y sur l'afficheur LCD
 * @return faux s'il y avait une mine, vrai sinon
 */
bool demine(char x, char y)
{
    bool mine= true;
    
    if (m_tabMines[y][x]== MINE)
    {
        mine=false;
    }
    if (m_tabMines[y][x]!= MINE)
    {
        mine=true;
        enleveTuilesAutour(x,y);
    }
    return mine; 
}
 
/*
 * @brief Dévoile les cases non minées autour de la tuile reçue en paramètre.
 * Cette méthode est appelée par demine().
 * @param char x, char y Les positions X et y sur l'afficheur LCD.
 * @return vrai s'il y avait une mine, faux sinon
 */
void enleveTuilesAutour(char x, char y)
{
    int i=0;
    int j=0; 
    int mine=0;
    mine = calculToucheCombien(y,x);
    if (mine==0)
    {
        m_tabVue[y][x]=32;
    }
    if (mine>0)
    {
        m_tabVue[y][x]= (mine+48);
        for ( i=-1;i<=1;i++)
        {
            for (j=-1;j<=1;j++)
            {
                if (m_tabMines[y+i][x+j]!=mine)
                {
                        strcpy (m_tabVue[y+i][x+j],m_tabMines[y+i][x+j]);
                }
            }
        }
    }         
     /*
    if(((y<3)&&(y>0)&&(x>0)&&(x<20)))
    {
        for(j=-1;j<=1;j++)
        {
            for(i=-1;i<=1;i++)
            {
                if ((m_tabMines[y+(j)][x+(i)]!=MINE)&&(m_tabMines[y+(j)][x+(i)]!=32))
                {
                    strcpy (m_tabVue[y+j][x+i],m_tabMines[y+(j)][x+i]);
                }
            }
        }
    }
     if((y==0)&&(x==0))
     {
        if((m_tabMines[y][x+1]!=MINE)&&(m_tabMines[y][x+1]!=32))
        {
            strcpy (m_tabVue[y][x+1],m_tabMines[y][x+1]);
        }
        for(j=0;j<=1;j++)
        {
            if((m_tabMines[y+1][x+(j)]=!MINE)&&(m_tabMines[y+1][x+j]!=32))
            {
                strcpy (m_tabVue[y][x+j],m_tabMines[y][x+j]);
            }
        }
     }*/
}
 
/*
 * @brief Vérifie si gagné. On a gagné quand le nombre de tuiles non dévoilées
 * est égal au nombre de mines. On augmente de 1 le nombre de mines si on a 
 * gagné.
 * @param int* pMines. Le nombre de mine.
 * @return vrai si gagné, faux sinon
 */
bool gagne(int* pMines)
{
    int nb_Tuile =0;
    bool gagne= false;
    int i=0;
    int j=0;
    for(i=0;i<NB_LIGNE;i++)
    {
        for(j=0;j<NB_COL;j++)
        {
            if(m_tabVue[i][j]==TUILE)
            {
              nb_Tuile++;  
            }
        }    
    } 
    if (*pMines == nb_Tuile)
    {
        gagne=true;
    }
    return gagne;
}
/*
 * @brief
 * @param rien
 * @return rien
 */
void afficheTabVue(void)
{
    int i=0;
    for(i=0;i<4;i++)//Cette commande affiche le contenu du tableau m_aliens[][] sur l'afficheur LCD.
    {
        lcd_gotoXY( 1, i+1);
        lcd_putMessage(m_tabMines[i]);  
    }
}