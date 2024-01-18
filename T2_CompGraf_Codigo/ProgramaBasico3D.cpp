// **********************************************************************
// PUCRS/Escola Politécnica
// COMPUTAÇÃO GRÁFICA
//
// Programa básico para criar aplicacoes 3D em OpenGL
//
// Marcio Sarroglia Pinho
// pinho@pucrs.br
// **********************************************************************

#include <iostream>
#include <cmath>
#include <ctime>
#include <iomanip>

using namespace std;


#ifdef WIN32
#include <windows.h>
#include <glut.h>
#else
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#endif

#ifdef __linux__
#include <glut.h>
#endif

#include "Temporizador.h"
#include "ListaDeCoresRGB.h"
#include "Ponto.h"
#include "Linha.h"

#include "SOIL/SOIL.h"
#include "TextureClass.h"

Temporizador T;
double AccumDeltaT=0;


GLfloat AspectRatio, angulo=15;

// Controle do modo de projecao
// 0: Projecao Paralela Ortografica; 1: Projecao Perspectiva
// A funcao "PosicUser" utiliza esta variavel. O valor dela eh alterado
// pela tecla 'p'
int ModoDeProjecao = 1;


// Controle do modo de projecao
// 0: Wireframe; 1: Faces preenchidas
// A funcao "Init" utiliza esta variavel. O valor dela eh alterado
// pela tecla 'e'
int ModoDeExibicao = 1;

double nFrames=0;
double TempoTotal=0;

//Pontos
Ponto Obs = {0,0,0};// Observador alteravel entre Tanque e TerceiraPessoa
Ponto Alvo = {0,0,10};// Alvo alteravel
bool modoTerceira = false;
Ponto Tanque = {-16.5, 0.5, -1.5}; // Obs no Tanque - Movel
Ponto TerceiraPessoa = {-30, 15, -4}; // Obs na TerceiraPessoa - Movel
Ponto olharFrente = {-6, 0.5, -1}; // Alvo quando Obs == Tanque
Ponto visaoTerceira = Tanque; // Alvo quando Obs == TerceiraPessoa
Ponto direcaoCanhaoBase = {1,0,0};
Ponto direcaoCanhao = {1,0,0};
int pontuacao = 0;

//Vetores
Ponto VetObsAlvo;
Ponto VetTerceiraPessoa;
Ponto VetTanqueFrente;
Ponto ProjDeslocX;
Ponto ProjDeslocY;

//Posicoes
Ponto CantoEsquerdo = {-20,-1,-10};// Canto do piso
Ponto PosicaoVeiculo = {-16.5,-0.5,-1.5};// Local do tanque
Ponto PosicaoCanhao = PosicaoVeiculo + Ponto(0,0.75,0);
Ponto PosicaoCanhaoEstrutura = PosicaoCanhao;
Ponto MinTanque = {-18, -1, -2.5};
Ponto MaxTanque = {-15, 0, -0.5};


//angulos
GLfloat anguloVisao = 5.0;
GLfloat anguloVeiculo = 0.0;
GLfloat anguloCanhao = 0.0;

//Disparo
GLfloat forca = 1.0;
GLfloat distancia = 0.0;
Ponto alcanceAux;
Ponto alcanceFinal;
bool disparo = false;
bool tiro = false;
bool configDisparo = false;
Ponto projetil = PosicaoCanhao;
float valorT = 0.0;

//Texturas
GLuint TexParede, TexGrama;
GLfloat imgCoordX1, imgCoordX2, imgCoordY1, imgCoordY2;

//Matriz
bool quadradosParede[25][15];

//Parede
Ponto MinParede = {6,-1,-10};
Ponto MaxParede = {6, 14, 15};

//Objetos
Ponto posicoesAmigos[10];
Ponto posicoesInimigos[10];




// *********************************************************************
//   ESTRUTURAS A SEREM USADAS PARA ARMAZENAR UM OBJETO 3D
// *********************************************************************



typedef struct // Struct para armazenar um tri‚ngulo
{
    Ponto P1, P2, P3, Normal;
    int rgb;
    void imprime()
    {
        cout << "P1 ";  P1.imprime(); cout << endl;
        cout << "P2 ";  P2.imprime(); cout << endl;
        cout << "P3 ";  P3.imprime(); cout << endl;
    }
} TTriangle;

// Classe para armazenar um objeto 3D
class Objeto3D
{
    TTriangle *faces; // vetor de faces
    unsigned int nFaces; // numero de faces do objeto

public:
    Objeto3D()
    {
        nFaces = 0;
        faces = NULL;
    }
    unsigned int getNFaces()
    {
        return nFaces;
    }
    void LeObjeto (char *Nome); // implementado fora da classe
    void ExibeObjeto(); // implementado fora da classe
};


Objeto3D *MundoVirtual;


// **********************************************************************
// void LeObjeto(char *Nome)
// **********************************************************************
void Objeto3D::LeObjeto (char *Nome)
{

    ifstream arq;
    arq.open(Nome, ios::in);
    if (!arq)
    {
        cout << "Erro na abertura do arquivo " << Nome << "." << endl;
        exit(1);
    }
    arq >> nFaces;
    faces = new TTriangle[nFaces];
    float x,y,z;
    int rgb;

    for (int i=1;i<nFaces;i++)
    {
        // Le os vertices
        arq >> x >> y >> z; // Vertice 1
        faces[i].P1.set(x,y,z);
        arq >> x >> y >> z; // Vertice 2
        faces[i].P2.set(x,y,z);
        arq >> x >> y >> z; // Vertice 3
        faces[i].P3.set(x,y,z);

        Ponto norma1, norma2, normaResult;
        norma1 = faces[i].P3 - faces[i].P2;
        norma2 = faces[i].P1 - faces[i].P2;
        norma1.versor();
        norma2.versor();
        ProdVetorial(norma1, norma2, normaResult);
        faces[i].Normal.set(normaResult.x,normaResult.y,normaResult.z);

        // ler o RGB da face
        arq >> std::hex >> rgb;
        faces[i].rgb = rgb;
    }
    arq.close();
}

// **********************************************************************
// void ExibeObjeto (TTriangle **Objeto)
// **********************************************************************
void Objeto3D::ExibeObjeto ()
{
    for(int i = 0; i<nFaces; i++)
    {

        TTriangle face = faces[i];

        defineCor(face.rgb%93);
        glBegin ( GL_TRIANGLES );
            glNormal3f(face.Normal.x,face.Normal.y,face.Normal.z);
            glVertex3f(face.P1.x,  face.P1.y, face.P1.z);
            glVertex3f(face.P2.x,  face.P2.y, face.P2.z);
            glVertex3f(face.P3.x,  face.P3.y, face.P3.z);
        glEnd();
    }


}



// **********************************************************************
//  void init(void)
//        Inicializa os parametros globais de OpenGL
// **********************************************************************

void init(void)
{
    glClearColor(1.0f, 1.0f, 0.0f, 1.0f); // Fundo de tela preto

    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE );
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    //glShadeModel(GL_FLAT);
    // Habilitar o uso de texturas
    glEnable ( GL_TEXTURE_2D );

    glColorMaterial ( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
    if (ModoDeExibicao) // Faces Preenchidas??
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    TexParede = LoadTexture("bricks.jpg");
    TexGrama = LoadTexture ("grass.jpg");

     // aloca memÛria
    MundoVirtual = new Objeto3D[1];
    // carrega o objeto 0
    MundoVirtual[0].LeObjeto ("tree.tri");
    Obs = Tanque;
    Alvo = olharFrente;


    //inicializando existencia das paredes
    for(int i = 0; i < 25; i++){
        for(int j =0; j < 15; j++){
            quadradosParede[i][j] = true;
        }
    }

    for(int i = 0; i<10; i++){
        if(i < 5){
            posicoesAmigos[i] = Ponto(-19 + i*4, -1, CantoEsquerdo.z + 2);
            posicoesInimigos[i] = Ponto(25 - i*4, -1, CantoEsquerdo.z + 2);
        }
        else
        {
            posicoesAmigos[i] = Ponto(-19 + (i-5)*4, -1, CantoEsquerdo.z + 24);
            posicoesInimigos[i] = Ponto(25 - (i-5)*4, -1, CantoEsquerdo.z + 24);
        }

    }
}


void RotacionaAoRedorDeUmPontoZ(float alfa, Ponto P)

{
    glTranslatef(P.x, P.y, P.z);

    glRotatef(alfa, 0,0,1);

    glTranslatef(-P.x, -P.y, -P.z);
}

void RotacionaAoRedorDeUmPontoY(float alfa, Ponto P)

{
    glTranslatef(P.x, P.y, P.z);

    glRotatef(alfa, 0,1,0);

    glTranslatef(-P.x, -P.y, -P.z);
}

// **********************************************************************
//
// **********************************************************************
void animate()
{
    double dt;
    dt = T.getDeltaT();
    AccumDeltaT += dt;
    TempoTotal += dt;
    nFrames++;

    if (AccumDeltaT > 1.0/30) // fixa a atualização da tela em 30
    {
        AccumDeltaT = 0;
        angulo+= 1;
        glutPostRedisplay();
    }
    if (TempoTotal > 5.0)
    {
        cout << "Tempo Acumulado: "  << TempoTotal << " segundos. " ;
        cout << "Nros de Frames sem desenho: " << nFrames << endl;
        cout << "FPS(sem desenho): " << nFrames/TempoTotal << endl;
        TempoTotal = 0;
        nFrames = 0;
    }
}


// **********************************************************************
//  void DesenhaCubo()
// **********************************************************************
void DesenhaCubo(float tamAresta)
{
    glBegin ( GL_QUADS );
    // Front Face
    glNormal3f(0,0,1);
    glVertex3f(-tamAresta/2, -tamAresta/2,  tamAresta/2);
    glVertex3f( tamAresta/2, -tamAresta/2,  tamAresta/2);
    glVertex3f( tamAresta/2,  tamAresta/2,  tamAresta/2);
    glVertex3f(-tamAresta/2,  tamAresta/2,  tamAresta/2);
    // Back Face
    glNormal3f(0,0,-1);
    glVertex3f(-tamAresta/2, -tamAresta/2, -tamAresta/2);
    glVertex3f(-tamAresta/2,  tamAresta/2, -tamAresta/2);
    glVertex3f( tamAresta/2,  tamAresta/2, -tamAresta/2);
    glVertex3f( tamAresta/2, -tamAresta/2, -tamAresta/2);
    // Top Face
    glNormal3f(0,1,0);
    glVertex3f(-tamAresta/2,  tamAresta/2, -tamAresta/2);
    glVertex3f(-tamAresta/2,  tamAresta/2,  tamAresta/2);
    glVertex3f( tamAresta/2,  tamAresta/2,  tamAresta/2);
    glVertex3f( tamAresta/2,  tamAresta/2, -tamAresta/2);
    // Bottom Face
    glNormal3f(0,-1,0);
    glVertex3f(-tamAresta/2, -tamAresta/2, -tamAresta/2);
    glVertex3f( tamAresta/2, -tamAresta/2, -tamAresta/2);
    glVertex3f( tamAresta/2, -tamAresta/2,  tamAresta/2);
    glVertex3f(-tamAresta/2, -tamAresta/2,  tamAresta/2);
    // Right face
    glNormal3f(1,0,0);
    glVertex3f( tamAresta/2, -tamAresta/2, -tamAresta/2);
    glVertex3f( tamAresta/2,  tamAresta/2, -tamAresta/2);
    glVertex3f( tamAresta/2,  tamAresta/2,  tamAresta/2);
    glVertex3f( tamAresta/2, -tamAresta/2,  tamAresta/2);
    // Left Face
    glNormal3f(-1,0,0);
    glVertex3f(-tamAresta/2, -tamAresta/2, -tamAresta/2);
    glVertex3f(-tamAresta/2, -tamAresta/2,  tamAresta/2);
    glVertex3f(-tamAresta/2,  tamAresta/2,  tamAresta/2);
    glVertex3f(-tamAresta/2,  tamAresta/2, -tamAresta/2);
    glEnd();

}
void DesenhaParalelepipedo()
{
    glPushMatrix();
        glTranslatef(0,0,-1);
        glScalef(1,1,2);
        glutSolidCube(2);
        //DesenhaCubo(1);
    glPopMatrix();
}

// **********************************************************************
// void DesenhaLadrilho(int corBorda, int corDentro)
// Desenha uma célula do piso.
// Eh possivel definir a cor da borda e do interior do piso
// O ladrilho tem largula 1, centro no (0,0,0) e está sobre o plano XZ
// **********************************************************************
void DesenhaLadrilho(int corBorda, int corDentro)
{
//    defineCor(corDentro);// desenha QUAD preenchido
    glColor3f(1,1,1);
    glBegin ( GL_QUADS );
        glNormal3f(0,1,0);
        glTexCoord2f(imgCoordX1, imgCoordY1);
        glVertex3f(0.0f,  0.0f, 0.0f);
        glTexCoord2f(imgCoordX2, imgCoordY1);
        glVertex3f(0.0f,  0.0f,  1.0f);
        glTexCoord2f(imgCoordX2, imgCoordY2);
        glVertex3f( 1.0f,  0.0f,  1.0f);
        glTexCoord2f(imgCoordX1, imgCoordY2);
        glVertex3f( 1.0f,  0.0f, 0.0f);
    glEnd();

    //defineCor(corBorda);
//    glColor3f(0,1,1);

    glBegin ( GL_LINE_STRIP );
        glNormal3f(0,1,0);
        glVertex3f(0.0f,  0.0f, 0.0f);
        glVertex3f(0.0f,  0.0f,  1.0f);
        glVertex3f( 1.0f,  0.0f,  1.0f);
        glVertex3f( 1.0f,  0.0f, 0.0f);
    glEnd();


}

// **********************************************************************
//
//
// **********************************************************************
void DesenhaPiso()
{
    imgCoordX1 = 0.0;
    imgCoordX2 = 0.0;
    imgCoordY1 = 0.0;
    imgCoordY2 = 0.0;
    srand(100); // usa uma semente fixa para gerar sempre as mesma cores no piso
    glPushMatrix();
    glTranslated(CantoEsquerdo.x, CantoEsquerdo.y, CantoEsquerdo.z);
    for(int x=0; x<50;x++)
    {
        imgCoordY1 = x * 0.020;
        imgCoordY2 = imgCoordY1 + 0.020;
        glPushMatrix();
        for(int z=0; z<25;z++)
        {
            imgCoordX1 = z * 0.040;
            imgCoordX2 = imgCoordX1 + 0.040;
            DesenhaLadrilho(MediumGoldenrod, rand()%40);
            glTranslated(0, 0, 1);
        }
        glPopMatrix();
        glTranslated(1, 0, 0);
    }
    glPopMatrix();
}

void DesenhaParede()
{
    imgCoordX1 = 0.0;
    imgCoordX2 = 0.0;
    imgCoordY1 = 0.0;
    imgCoordY2 = 0.0;
    glPushMatrix();
        glTranslatef(5,0,0);
        glRotatef(90,0,0,1);
        glTranslatef(19,0,0);
        srand(100); // usa uma semente fixa para gerar sempre as mesma cores no piso
        glTranslated(CantoEsquerdo.x, CantoEsquerdo.y, CantoEsquerdo.z);
        for(int y=0; y<15;y++){
            imgCoordY1 = y * 0.060;
            imgCoordY2 = imgCoordY1 + 0.060;
            glPushMatrix();
            for(int z=0; z<25;z++){
                imgCoordX1 = z * 0.040;
                imgCoordX2 = imgCoordX1 + 0.040;
                if(quadradosParede[z][y]){
                    DesenhaLadrilho(MediumGoldenrod, rand()%40);
                }
                glTranslated(0, 0, 1);
            }
            glPopMatrix();
            glTranslated(1, 0, 0);
        }
    glPopMatrix();
}
// **********************************************************************
//  void DefineLuz(void)
// **********************************************************************
void DefineLuz(void)
{
  // Define cores para um objeto dourado
  GLfloat LuzAmbiente[]   = {0.4, 0.4, 0.4 } ;
  GLfloat LuzDifusa[]   = {0.7, 0.7, 0.7};
  GLfloat LuzEspecular[] = {0.9f, 0.9f, 0.9 };
  GLfloat PosicaoLuz0[]  = {0.0f, 3.0f, 5.0f };  // Posição da Luz
  GLfloat Especularidade[] = {1.0f, 1.0f, 1.0f};

   // ****************  Fonte de Luz 0

 glEnable ( GL_COLOR_MATERIAL );

   // Habilita o uso de iluminação
  glEnable(GL_LIGHTING);

  // Ativa o uso da luz ambiente
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, LuzAmbiente);
  // Define os parametros da luz número Zero
  glLightfv(GL_LIGHT0, GL_AMBIENT, LuzAmbiente);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, LuzDifusa  );
  glLightfv(GL_LIGHT0, GL_SPECULAR, LuzEspecular  );
  glLightfv(GL_LIGHT0, GL_POSITION, PosicaoLuz0 );
  glEnable(GL_LIGHT0);

  // Ativa o "Color Tracking"
  glEnable(GL_COLOR_MATERIAL);

  // Define a reflectancia do material
  glMaterialfv(GL_FRONT,GL_SPECULAR, Especularidade);

  // Define a concentraçãoo do brilho.
  // Quanto maior o valor do Segundo parametro, mais
  // concentrado será o brilho. (Valores válidos: de 0 a 128)
  glMateriali(GL_FRONT,GL_SHININESS,51);

}
// **********************************************************************

void MygluPerspective(float fieldOfView, float aspect, float zNear, float zFar )
{
    //https://stackoverflow.com/questions/2417697/gluperspective-was-removed-in-opengl-3-1-any-replacements/2417756#2417756
    // The following code is a fancy bit of math that is equivilant to calling:
    // gluPerspective( fieldOfView/2.0f, width/height , 0.1f, 255.0f )
    // We do it this way simply to avoid requiring glu.h
    //GLfloat zNear = 0.1f;
    //GLfloat zFar = 255.0f;
    //GLfloat aspect = float(width)/float(height);
    GLfloat fH = tan( float(fieldOfView / 360.0f * 3.14159f) ) * zNear;
    GLfloat fW = fH * aspect;
    glFrustum( -fW, fW, -fH, fH, zNear, zFar );
}
// **********************************************************************
//  void PosicUser()
// **********************************************************************
void PosicUser()
{

    // Define os parâmetros da projeção Perspectiva
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Define o volume de visualização sempre a partir da posicao do
    // observador
    if (ModoDeProjecao == 0)
        glOrtho(-10, 10, -10, 10, 0, 7); // Projecao paralela Orthografica
    else MygluPerspective(90,AspectRatio,0.01,100); // Projecao perspectiva

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(Obs.x, Obs.y, Obs.z,   // Posição do Observador
              Alvo.x, Alvo.y, Alvo.z,     // Posição do Alvo
              0.0f,1.0f,0.0f);

}
// **********************************************************************
//  void reshape( int w, int h )
//		trata o redimensionamento da janela OpenGL
//
// **********************************************************************
void reshape( int w, int h )
{

	// Evita divisão por zero, no caso de uam janela com largura 0.
	if(h == 0) h = 1;
    // Ajusta a relação entre largura e altura para evitar distorção na imagem.
    // Veja função "PosicUser".
	AspectRatio = 1.0f * w / h;
	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Seta a viewport para ocupar toda a janela
    glViewport(0, 0, w, h);
    //cout << "Largura" << w << endl;

	PosicUser();

}


void desenhaTanque()
{

    glPushMatrix();

        glDisable(GL_TEXTURE_2D);
        defineCor(DarkOliveGreen);
        RotacionaAoRedorDeUmPontoY(anguloVeiculo,Tanque);
        glTranslatef(PosicaoVeiculo.x, PosicaoVeiculo.y, PosicaoVeiculo.z);
        glRotatef(anguloVeiculo, 0, 1, 0);
        glScalef(3,1,2);
        glutSolidCube(1);
    glPopMatrix();

    glPushMatrix();
        defineCor(ForestGreen);
        RotacionaAoRedorDeUmPontoY(anguloVeiculo,Tanque);
        glTranslatef(PosicaoCanhaoEstrutura.x, PosicaoCanhaoEstrutura.y, PosicaoCanhaoEstrutura.z);

        glRotatef(anguloVeiculo, 0, 1, 0);
        glRotatef(anguloCanhao, 0, 0, 1);
        glTranslatef(0.75,0,0);
        glScalef(2.0, 0.5, 0.5);
        glutSolidCube(1);
    glPopMatrix();
}




bool colisaoParede()
{
    if(projetil.x >= 6){
        if((projetil.z > -10 && projetil.z < 15)&& projetil.y < 15 && projetil.y >= 0)
        {
            int yAux = (int)projetil.y + 1;
            int zAux = (int)projetil.z + 10;
            if(quadradosParede[zAux][yAux])
            {
                quadradosParede[zAux][yAux] = false;
                if(yAux-1 >= 0){quadradosParede[zAux][yAux-1] = false;}
                if(zAux-1 >= 0){quadradosParede[zAux-1][yAux] = false;}
                if(yAux+1 < 15){quadradosParede[zAux][yAux+1] = false;}
                if(zAux+1 < 25){quadradosParede[zAux+1][yAux] = false;}
                if(yAux-1 >= 0 && zAux-1 >= 0){quadradosParede[zAux-1][yAux-1] = false;}
                if(yAux+1 < 15 && zAux+1 < 25){quadradosParede[zAux+1][yAux+1] = false;}
                if(yAux-1 >= 0 && zAux+1 < 25){quadradosParede[zAux+1][yAux-1] = false;}
                if(yAux+1 < 15 && zAux-1 >= 0){quadradosParede[zAux-1][yAux+1] = false;}
                pontuacao += 5;
                cout << "Pontuacao atual = " << pontuacao << endl;
                return true;
            }
        }
    }
    return false;
}

//bool colisaoObjetos()
//{
//    Ponto aux,vet1,vet2;
//    if(projetil.x >= 6)// testa inimigos
//    {
////        for(int i = 0; i<10; i++){
////            aux = posicoesInimigos[i] + Ponto(0,10,0);
////            vet1 = aux - posicoesInimigos[i];
////            vet2 = projetil - posicoesInimigos[i];
////            float esc = ProdEscalar(vet1,vet2);
////            if(esc >= 0.8 && esc <= 1.0){
////                posicoesInimigos[i].y = -100;
////                pontuacao += 10;
////                return true;
////            }
////        }
//    }
//    else// testa amigos
//    {
//        for(int i = 0; i<10; i++){
//            aux = posicoesInimigos[i] + Ponto(0,10,0);
//            vet1 = aux - posicoesAmigos[i];
//            vet2 = projetil - posicoesAmigos[i];
//            float esc = ProdEscalar(vet1,vet2);
//            if(esc >= 0.8 && esc <= 1.0){
//                posicoesAmigos[i].y = -100;
//                pontuacao -= 10;
//                return true;
//            }
//        }
//    }
//    return false;
//}

void trajetoria(float t)
{
    float aux = 1-t;
    Ponto parteP0 = PosicaoCanhao * pow(aux,2);
    Ponto parteP1 = alcanceAux * (2*aux*t);
    Ponto parteP2 = alcanceFinal * pow(t,2);
    projetil = parteP0 + parteP1 + parteP2;

    glPushMatrix();
        glColor3f(0.5,0.5,0.5);
        glTranslatef(projetil.x,projetil.y,projetil.z);
        glutSolidSphere(0.3,15,15);
    glPopMatrix();

    if(projetil.y < -1 || projetil.x > 50 || projetil.x < -30 || projetil.z > 30 || projetil.z < -40)
    {
        disparo = false;
        valorT = 0.0;
    }
    if(colisaoParede()){
        disparo = false;
        valorT = 0.0;
    }
}
// **********************************************************************
//  void display( void )
// **********************************************************************

void display( void )
{
    if(!modoTerceira)
    {
        Obs = Tanque;
        Alvo = olharFrente;
    }
    else
    {
        Obs = TerceiraPessoa;
        Alvo = visaoTerceira;
    }
    VetTanqueFrente = olharFrente - Tanque;
    VetTerceiraPessoa = visaoTerceira - TerceiraPessoa;
    VetObsAlvo = Alvo - Obs;

    if(configDisparo)
    {
        direcaoCanhao = Ponto (1,0,0);
        direcaoCanhao.rotacionaZ(anguloCanhao);
        direcaoCanhao.rotacionaY(anguloVeiculo);
        configDisparo = false;
    }

        alcanceAux = PosicaoCanhao + direcaoCanhao * forca;
        distancia = 2 * forca * cos(anguloCanhao * 3.14/180);
        alcanceFinal = PosicaoCanhao + Ponto(distancia, 0, 0);

        Ponto vetAuxCanhao = alcanceAux - PosicaoCanhao;
        vetAuxCanhao.rotacionaY(anguloVeiculo);
        alcanceAux = PosicaoCanhao + vetAuxCanhao;

        Ponto vetDistCanhao = alcanceFinal - PosicaoCanhao;
        vetDistCanhao.rotacionaY(anguloVeiculo*2);
        alcanceFinal = PosicaoCanhao + vetDistCanhao;



	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	DefineLuz();

	PosicUser();

	glMatrixMode(GL_MODELVIEW);





/////////////////////////////////////////////////
    glPushMatrix();
//        glTranslatef(PosicaoVeiculo.x, PosicaoVeiculo.y+1, PosicaoVeiculo.z);
        glColor3f(1,0,1);
        glTranslatef(alcanceAux.x, alcanceAux.y, alcanceAux.z);
        glutSolidSphere(0.350,15,15);
    glPopMatrix();

    glPushMatrix();
//        glTranslatef(PosicaoVeiculo.x, PosicaoVeiculo.y+1, PosicaoVeiculo.z);
        glColor3f(0,0,0);
        glTranslatef(alcanceFinal.x, alcanceFinal.y, alcanceFinal.z);
        glutSolidSphere(0.350,15,15);
    glPopMatrix();

    glPushMatrix();
//        glTranslatef(PosicaoVeiculo.x, PosicaoVeiculo.y+1, PosicaoVeiculo.z);
        glColor3f(0,0,1);
        glTranslatef(PosicaoCanhao.x, PosicaoCanhao.y, PosicaoCanhao.z);
        glutSolidSphere(0.350,15,15);
    glPopMatrix();


//    glPushMatrix();
//        glColor3f(1,0,1);
//        glTranslatef(MinParede.x, MinParede.y, MinParede.z);
//        glutSolidSphere(0.350,15,15);
//    glPopMatrix();
//
//    glPushMatrix();
//        glColor3f(0,1,0);
//        glTranslatef(MaxParede.x, MaxParede.y, MaxParede.z);
//        glutSolidSphere(0.350,15,15);
//    glPopMatrix();
//////////////////////////////////////////////////////




    glEnable(GL_TEXTURE_2D);

    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, TexGrama);
        DesenhaPiso();
    glPopMatrix();


    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, TexParede);
        DesenhaParede();
    glPopMatrix();

    for(int i = 0; i< 10; i++)
    {
        Ponto desloca = posicoesAmigos[i];
        if(desloca.y == -1)
        {
            glPushMatrix();
                glTranslatef(desloca.x,desloca.y,desloca.z);
                MundoVirtual[0].ExibeObjeto();
            glPopMatrix();
        }

        desloca = posicoesInimigos[i];
        if(desloca.y == -1)
        {
            glPushMatrix();
                glTranslatef(desloca.x,desloca.y,desloca.z);
                MundoVirtual[0].ExibeObjeto();
            glPopMatrix();
        }

    }

    if(disparo)
    {
        trajetoria(valorT);

        valorT += 0.050;
    }

    desenhaTanque();
	glutSwapBuffers();
}



// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
//
//
// **********************************************************************
void keyboard ( unsigned char key, int x, int y )
{
    Ponto aux;
	switch ( key )
	{

    case 27:        // Termina o programa qdo
      exit ( 0 );   // a tecla ESC for pressionada
      break;
    case 'p':
            ModoDeProjecao = !ModoDeProjecao;
            glutPostRedisplay();
            break;
    case 'e':
            ModoDeExibicao = !ModoDeExibicao;
            init();
            glutPostRedisplay();
            break;
    case 'w':

            Tanque = Tanque + (VetTanqueFrente)*0.2;
            olharFrente = olharFrente + (VetTanqueFrente)*0.2;
            PosicaoVeiculo = PosicaoVeiculo + (VetTanqueFrente)*0.2;
            PosicaoCanhaoEstrutura = PosicaoCanhaoEstrutura + (VetTanqueFrente)*0.2;
            PosicaoCanhao = PosicaoCanhao + (VetTanqueFrente)*0.2;
            break;
    case 'a':
            aux = VetTanqueFrente;
            aux.rotacionaY(anguloVisao*2);
            olharFrente = Tanque + aux;
            anguloVeiculo += 5;
            if(anguloVeiculo >= 180){anguloVeiculo = 0;}
            configDisparo = true;
            break;
    case 's':
            Tanque = Tanque - (VetTanqueFrente)*0.2;
            olharFrente = olharFrente - (VetTanqueFrente)*0.2;
            PosicaoVeiculo = PosicaoVeiculo - (VetTanqueFrente)*0.2;
            PosicaoCanhaoEstrutura = PosicaoCanhaoEstrutura - (VetTanqueFrente)*0.2;
            PosicaoCanhao = PosicaoCanhao - (VetTanqueFrente)*0.2;
            break;
    case 'd':
            aux = VetTanqueFrente;
            aux.rotacionaY(-anguloVisao*2);
            olharFrente = Tanque + aux;
            anguloVeiculo -= 5;
            if(anguloVeiculo <= -180){anguloVeiculo = 0;}
            configDisparo = true;
            break;
    case 'm':
        disparo = true;
        break;

    case 'u':
        if(modoTerceira)
        {
            aux = VetTerceiraPessoa;
            glPushMatrix();
                glRotatef(anguloVisao,0,1,0);
                aux.rotacionaZ(anguloVisao);
                visaoTerceira = TerceiraPessoa + aux;
            glPopMatrix();
        }
            break;
    case 'U':
        if(modoTerceira)
        {
            aux = VetTerceiraPessoa;
            glPushMatrix();
                glRotatef(anguloVisao,0,1,0);
                aux.rotacionaZ(-anguloVisao);
                visaoTerceira = TerceiraPessoa + aux;
            glPopMatrix();
        }
            break;
    case 't': // TROCA OBSERVADOR
        if (!modoTerceira){
            modoTerceira = !modoTerceira;

            TerceiraPessoa = Ponto(Tanque.x-5,Tanque.y+10,Tanque.z+20);
            Obs = TerceiraPessoa;
            visaoTerceira = Ponto(Tanque.x,Tanque.y,Tanque.z);
            Alvo = visaoTerceira;
        }
        else{
            modoTerceira = !modoTerceira;
            Obs = Tanque;
            Alvo = olharFrente;
        }
        break;
    case 'f': // aumenta forca do tiro
        if(forca < 50.0){forca += 1.0;} else {forca = 50.0;}
        configDisparo = true;
        break;
    case 'F': // diminui forca do tiro
        if(forca <= 50.0 && forca > 1.0){forca -= 1.0;} else {forca = 1.0;}
        configDisparo = true;
        break;
    case 'c': // sobe canhao
        if(anguloVeiculo == 0.0 && anguloCanhao < 90.0){
            anguloCanhao += 1.0;
            configDisparo = true;
        }
        break;
    case 'C':// baixa canhao
        if(anguloVeiculo == 0.0 && anguloCanhao > 0.0){
            anguloCanhao -= 1.0;
            configDisparo = true;
        }
        break;
    default:
            cout << key;
    break;
  }
}

// **********************************************************************
//  void arrow_keys ( int a_keys, int x, int y )
//
//
// **********************************************************************
void arrow_keys ( int a_keys, int x, int y )
{
    Ponto aux;
	switch ( a_keys )
	{
		case GLUT_KEY_UP:

            if(modoTerceira)
            {
                TerceiraPessoa = TerceiraPessoa + (VetTerceiraPessoa)*0.2;
                visaoTerceira = visaoTerceira + (VetTerceiraPessoa)*0.2;
            }
			break;
	    case GLUT_KEY_DOWN:

            if(modoTerceira)
            {
                TerceiraPessoa = TerceiraPessoa - (VetTerceiraPessoa)*0.2;
                visaoTerceira = visaoTerceira - (VetTerceiraPessoa)*0.2;
            }
			break;
        case GLUT_KEY_LEFT:
            if(modoTerceira)
            {
                aux = VetTerceiraPessoa;
                aux.rotacionaY(anguloVisao);
                visaoTerceira = Obs + aux;
            }
            break;
        case GLUT_KEY_RIGHT:
            if(modoTerceira)
            {
                aux = VetTerceiraPessoa;
                aux.rotacionaY(-anguloVisao);
                visaoTerceira = Obs + aux;
            }
            break;
		default:
			break;
	}
}

// **********************************************************************
//  void main ( int argc, char** argv )
//
//
// **********************************************************************
int main ( int argc, char** argv )
{
	glutInit            ( &argc, argv );
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
	glutInitWindowPosition (0,0);
	glutInitWindowSize  ( 700, 700 );
	glutCreateWindow    ( "Computacao Grafica - Exemplo Basico 3D" );

	init ();
    //system("pwd");

	glutDisplayFunc ( display );
	glutReshapeFunc ( reshape );
	glutKeyboardFunc ( keyboard );
	glutSpecialFunc ( arrow_keys );
	glutIdleFunc ( animate );

	glutMainLoop ( );
	return 0;
}



