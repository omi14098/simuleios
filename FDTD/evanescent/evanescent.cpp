/*-------------evanescent.cpp-------------------------------------------------//
*
*              Finite Difference Time Domain -- evanescent field test
*
* Purpose: To replicate the results of our invisible lense raytracer with
*          FDTD. Woo!
*
*   Notes: Most of this is coming from the following link:
*             http://www.eecs.wsu.edu/~schneidj/ufdtd/chap3.pdf
*             http://www.eecs.wsu.edu/~schneidj/ufdtd/chap8.pdf
*
*-----------------------------------------------------------------------------*/

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

static const size_t spacey = 300;
static const size_t spacex = 500;
static const size_t losslayer = 20;

struct Bound{
    int x,y;
};

struct Loss{
    std::vector <double> EzH = std::vector<double>(spacex * spacey, 0),
                         EzE = std::vector<double>(spacex * spacey, 0),
                         HyE = std::vector<double>(spacex * spacey, 0),
                         HyH = std::vector<double>(spacex * spacey, 0),
                         HxE = std::vector<double>(spacex * spacey, 0),
                         HxH = std::vector<double>(spacex * spacey, 0);
};

struct Loss1d{
    std::vector <double> EzH = std::vector<double>(spacex, 0),
                         EzE = std::vector<double>(spacex, 0),
                         HyE = std::vector<double>(spacex, 0),
                         HyH = std::vector<double>(spacex, 0);
};

struct Field{
    std::vector <double> Hx = std::vector<double>(spacex * spacey, 0),
                         Hy = std::vector<double>(spacex * spacey, 0),
                         Ez = std::vector<double>(spacex * spacey, 0);

    std::vector <double> Hy1d = std::vector<double>(spacex + losslayer, 0),
                         Ez1d = std::vector<double>(spacex + losslayer, 0),
                         Hy1d2 = std::vector<double>(spacex + losslayer, 0),
                         Ez1d2 = std::vector<double>(spacex + losslayer, 0);


    // 6 elements, 3 spacial elements away from border and 2 time elements of
    // those spatial elements
    std::vector <double> Etop = std::vector<double>(3 * 2 * spacex, 0),
                         Ebot = std::vector<double>(3 * 2 * spacex, 0),
                         Eleft = std::vector<double>(3 * 2 * spacey, 0),
                         Eright = std::vector<double>(3 * 2 * spacey, 0);

    int t;
};

#define EzH(i, j) EzH[(i) + (j) *  spacex]
#define EzE(i, j) EzE[(i) + (j) *  spacex]
#define HyH(i, j) HyH[(i) + (j) *  spacex]
#define HyE(i, j) HyE[(i) + (j) *  spacex]
#define HxH(i, j) HxH[(i) + (j) *  spacex]
#define HxE(i, j) HxE[(i) + (j) *  spacex]
#define Hx(i, j) Hx[(i) + (j) *  spacex]
#define Hy(i, j) Hy[(i) + (j) *  spacex]
#define Ez(i, j) Ez[(i) + (j) *  spacex]
#define Etop(k, j, i) Etop[(i) * 6 + (j) * 3 + (k)]
#define Ebot(k, j, i) Ebot[(i) * 6 + (j) * 3 + (k)]
#define Eleft(i, j, k) Eleft[(k) * 6 + (j) * 3 + (i)]
#define Eright(i, j, k) Eright[(k) * 6 + (j) * 3 + (i)]


void FDTD(Field EM,
          const int final_time, const double eps,
          std::ofstream& output);

// Adding ricker solutuion
double ricker(int time, int loc, double Cour);

// Adding plane wave
double planewave(int time, int loc, double Cour, int ppw);

// 2 dimensional functions for E / H movement
void Hupdate2d(Field &EM, Loss &lass, int t);
void Eupdate2d(Field &EM, Loss &lass, int t);

// 1 dimensional update functions for E / H
void Hupdate1d(Field &EM, Loss1d &lass1d, int t);
void Eupdate1d(Field &EM, Loss1d &lass1d, int t);

// Creating loss
void createloss2d(Loss &lass, double eps, double Cour, double loss);
void createloss1d(Loss1d &lass1d, double eps, double Cour, double loss);

// Total Field Scattered Field (TFSF) boundaries
void TFSF(Field &EM, Loss &lass, Loss1d &lass1d, double Cour);
void TFSF2(Field &EM, Loss &lass, Loss1d &lass1d, double Cour);

// Checking Absorbing Boundary Conditions (ABS)
void ABCcheck(Field &EM, Loss &lass);

/*----------------------------------------------------------------------------//
* MAIN
*-----------------------------------------------------------------------------*/

int main(){

    // defines output
    std::ofstream output("evanescent.dat", std::ofstream::out);

    int final_time = 4001;
    double eps = 377.0;

    // define initial E and H fields
    // std::vector<double> Ez(space, 0.0), Hy(space, 0.0);
    Field EM;
    EM.t = 0;

    FDTD(EM, final_time, eps, output);

}

/*----------------------------------------------------------------------------//
* SUBROUTINES
*-----------------------------------------------------------------------------*/

// This is the function we writs the bulk of the code in
void FDTD(Field EM,
          const int final_time, const double eps,
          std::ofstream& output){

    double loss = 0.00;
    double Cour = 1 / sqrt(2);

    Loss lass;
    createloss2d(lass, eps, Cour, loss);
    Loss1d lass1d;
    createloss1d(lass1d, eps, Cour, loss);

    // Time looping
    for (int t = 0; t < final_time; t++){

        Hupdate2d(EM, lass, t);
        TFSF(EM, lass, lass1d, Cour);
        TFSF2(EM, lass, lass1d, Cour);
        Eupdate2d(EM,lass,t);
        ABCcheck(EM, lass);
        //EM.Ez(200,100) = ricker(t, 0, Cour);

        // Outputting to a file
        int check = 50;
        if (t % check == 0){
            for (size_t dx = 0; dx < spacex; dx++){
                for (size_t dy = 0; dy < spacey; dy++){
                    output << t << '\t' << dx <<'\t' << dy << '\t'
                           << EM.Ez(dx, dy) << '\t' << EM.Hy(dx, dy)
                           << '\t' << EM.Hx(dx, dy) << '\t' << '\n';
                }
            }

            output << '\n' << '\n';
        }

    }
}

// Adding the ricker solution
double ricker(int time, int loc, double Cour){
    double Ricky;
    double temp_const = 3.14159*((Cour*(double)time - (double)loc)/20.0 - 1.0);
    temp_const = temp_const * temp_const;
    Ricky = (1.0 - 2.0 * temp_const) * exp(-temp_const);
    return Ricky;

}

// 2 dimensional functions for E / H movement
void Hupdate2d(Field &EM, Loss &lass, int t){
    // update magnetic field, x direction
    #pragma omp parallel for
    for (size_t dx = 0; dx < spacex; dx++){
        for (size_t dy = 0; dy < spacey - 1; dy++){
           EM.Hx(dx,dy) = lass.HxH(dx,dy) * EM.Hx(dx, dy)
                       - lass.HxE(dx,dy) * (EM.Ez(dx,dy + 1)
                                            - EM.Ez(dx,dy));
        }
    }

    // update magnetic field, y direction
    #pragma omp parallel for
    for (size_t dx = 0; dx < spacex - 1; dx++){
        for (size_t dy = 0; dy < spacey; dy++){
           EM.Hy(dx,dy) = lass.HyH(dx,dy) * EM.Hy(dx,dy)
                      + lass.HyE(dx,dy) * (EM.Ez(dx + 1,dy)
                                            - EM.Ez(dx,dy));
        }
    }

    //return EM;

}


void Eupdate2d(Field &EM, Loss &lass, int t){
    // update electric field
    #pragma omp parallel for
    for (size_t dx = 1; dx < spacex - 1; dx++){
        for (size_t dy = 1; dy < spacey - 1; dy++){
           EM.Ez(dx,dy) = lass.EzE(dx,dy) * EM.Ez(dx,dy)
                       + lass.EzH(dx,dy) * ((EM.Hy(dx, dy)
                                         - EM.Hy(dx - 1, dy))
                                         - (EM.Hx(dx,dy)
                                         - EM.Hx(dx, dy - 1)));
        }
    }
    //return EM;
}

// 1 dimensional update functions for E / H
void Hupdate1d(Field &EM, Loss1d &lass1d, int t){
    // update magnetic field, y direction
    #pragma omp parallel for
    for (size_t dx = 0; dx < spacex - 1; dx++){
        EM.Hy1d[dx] = lass1d.HyH[dx] * EM.Hy1d[dx]
                  + lass1d.HyE[dx] * (EM.Ez1d[dx + 1] - EM.Ez1d[dx]);
    }

    //return EM;
}

void Eupdate1d(Field &EM, Loss1d &lass1d, int t){
    // update electric field, y direction
    for (size_t dx = 1; dx < spacex - 1; dx++){
        EM.Ez1d[dx] = lass1d.EzE[dx] * EM.Ez1d[dx]
                  + lass1d.EzH[dx] * (EM.Hy1d[dx] - EM.Hy1d[dx - 1]);
    }

    //return EM;

}

// Creating loss
void createloss2d(Loss &lass, double eps, double Cour, double loss){

    //double var = 3.0;
    double fiber_var = 3.0;
    int buffer = 5;
    //int lens_offset = 300;
    // double radius = 50, dist, dist2;
    //int sourcex = 0, sourcex2 = 60;
    //int sourcey = 250, sourcey2 = 250;

    for (int dx = 0; dx < spacex; dx++){
        for (int dy = 0; dy < spacey; dy++){

             /*
             dist = sqrt((dx - sourcex)*(dx - sourcex) 
                       + (dy - sourcey)*(dy - sourcey)); 
             dist2 = sqrt((dx - sourcex2)*(dx - sourcex2) 
                        + (dy - sourcey2)*(dy - sourcey2)); 
             */

            //if ((dy > 65 && dy < 115) || (dy > 125 && dy < 175) ||
            //    (dy > 185 && dy < 235)){
            if (((dx + dy) < 400 && (dx+dy > 350) && dy < 275 && dy > 75) || 
                (dy > 225 && dy < 275 && dx < 125) ||
                (dy > 75 && dy < 125 && dx > 275 && dx < 400)){
                lass.EzH(dx, dy) = Cour * eps / (fiber_var * fiber_var);
                lass.EzE(dx, dy) = 1.0;
                lass.HyH(dx, dy) = 1.0;
                lass.HyE(dx, dy) = Cour * (1.0 / eps);
                lass.HxE(dx, dy) = Cour * (1.0 / eps);
                lass.HxH(dx, dy) = 1.0;

            }

            else if (((dx + dy) < 450 + buffer && (dx+dy > 400 + buffer) && 
                       dy < 225 && dy > 75)||
                      (dy > 125 + buffer && dy < 175 + buffer && 
                       dx > 275 && dx < 400)){
                lass.EzH(dx, dy) = Cour * eps / (fiber_var * fiber_var);
                lass.EzE(dx, dy) = 1.0;
                lass.HyH(dx, dy) = 1.0;
                lass.HyE(dx, dy) = Cour * (1.0 / eps);
                lass.HxE(dx, dy) = Cour * (1.0 / eps);
                lass.HxH(dx, dy) = 1.0;

            }

            /*
            // if (dy > 125 && dy < 175){
            else if (dist < radius && dist2 < radius){            
                lass.EzH(dx, dy) = Cour * eps / (var * var);
                lass.EzE(dx, dy) = 1.0;
                lass.HyH(dx, dy) = 1.0;
                lass.HyE(dx, dy) = Cour * (1.0 / eps);
                lass.HxE(dx, dy) = Cour * (1.0 / eps);
                lass.HxH(dx, dy) = 1.0;

            }
            */
            else{
                lass.EzH(dx, dy) = Cour * eps;
                lass.EzE(dx, dy) = 1.0;
                lass.HyH(dx, dy) = 1.0;
                lass.HyE(dx, dy) = Cour * (1.0 / eps);
                lass.HxE(dx, dy) = Cour * (1.0 / eps);
                lass.HxH(dx, dy) = 1.0;
           }
        }
    }


    //return lass;
}
void createloss1d(Loss1d &lass1d, double eps, double Cour, double loss){

    double depth, lossfactor;

    for (size_t dx = 0; dx < spacex; dx++){
        if (dx < spacex - 1 - losslayer){
            lass1d.EzH[dx] = Cour * eps;
            lass1d.EzE[dx] = 1.0;
            lass1d.HyH[dx] = 1.0;
            lass1d.HyE[dx] = Cour / eps;
        }
        else{
            depth = dx - spacex - 1 - losslayer - 0.5;
            lossfactor = loss * pow(depth / (double)losslayer, 2);
            lass1d.EzH[dx] = Cour * eps / (1.0 + lossfactor);
            lass1d.EzE[dx] = (1.0 - lossfactor) / (1.0 + lossfactor);
            depth += 0.5;
            lass1d.HyH[dx] = (1.0 - lossfactor) / (1.0 + lossfactor);
            lass1d.HyE[dx] = Cour / eps / (1.0 + lossfactor);

        }
    }


    //return lass1d;

}

// TFSF boundaries
void TFSF(Field &EM, Loss &lass, Loss1d &lass1d, double Cour){

    int dx, dy;

    // TFSF boundary
    Bound first, last;
    first.x = 10; last.x = 290;
    first.y = 65; last.y = 115;

    // Update along right edge!
    dx = last.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Hy(dx,dy) += lass.HyE(dx, dy) * EM.Ez1d[dx];
    }

    // Updating along left edge
    dx = first.x - 1;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Hy(dx,dy) -= lass.HyE(dx, dy) * EM.Ez1d[dx+1];
    }

    // Updating along top
    dy = last.y;
    for (int dx = first.x; dx <= last.x; dx++){
        EM.Hx(dx,dy) -= lass.HxE(dx, dy) * EM.Ez1d[dx];
    }

    // Update along bot
    dy = first.y - 1;
    for (int dx = first.x; dx <= last.x; dx++){
        EM.Hx(dx,dy) += lass.HxE(dx, dy) * EM.Ez1d[dx];
    }

    // Insert 1d grid stuff here. Update magnetic and electric field
    Hupdate1d(EM, lass1d, EM.t);
    Eupdate1d(EM, lass1d, EM.t);
    //EM.Ez1d[10] = ricker(EM.t,0, Cour);
    //EM.Ez1d[10] = planewave(EM.t, 15, Cour, 10);
    //EM.Ez1d[290] = planewave(EM.t, 15, Cour, 10);
    EM.t++;
    std::cout << EM.t << '\n';

    // Check mag instead of ricker.
    // Update along right
    dx = last.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Ez(dx, dy) += lass.EzH(dx, dy) * EM.Hy1d[dx];
    }

    // Updating Ez along left
    dx = first.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Ez(dx, dy) -= lass.EzH(dx, dy) * EM.Hy1d[dx - 1];
    }

    //return EM;

}

// second TFSF boundary
void TFSF2(Field &EM, Loss &lass, Loss1d &lass1d, double Cour){

    int dx, dy;

    // TFSF boundary
    Bound first, last;
    first.x = 10; last.x = 290;
    first.y = 225; last.y = 275;

    // Update along right edge!
    dx = last.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Hy(dx,dy) += lass.HyE(dx, dy) * EM.Ez1d2[dx];
    }

    // Updating along left edge
    dx = first.x - 1;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Hy(dx,dy) -= lass.HyE(dx, dy) * EM.Ez1d2[dx+1];
    }

    // Updating along top
    dy = last.y;
    for (int dx = first.x; dx <= last.x; dx++){
        EM.Hx(dx,dy) -= lass.HxE(dx, dy) * EM.Ez1d2[dx];
    }

    // Update along bot
    dy = first.y - 1;
    for (int dx = first.x; dx <= last.x; dx++){
        EM.Hx(dx,dy) += lass.HxE(dx, dy) * EM.Ez1d2[dx];
    }

    // Insert 1d grid stuff here. Update magnetic and electric field
    Hupdate1d(EM, lass1d, EM.t);
    Eupdate1d(EM, lass1d, EM.t);
    //EM.Ez1d2[10] = ricker(EM.t,0, Cour);
    EM.Ez1d2[10] = planewave(EM.t, 15, Cour, 10);
    //EM.Ez1d2[290] = planewave(EM.t, 15, Cour, 10);
    EM.t++;
    std::cout << EM.t << '\n';

    // Check mag instead of ricker.
    // Update along right
    dx = last.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Ez(dx, dy) += lass.EzH(dx, dy) * EM.Hy1d2[dx];
    }

    // Updating Ez along left
    dx = first.x;
    for (int dy = first.y; dy <= last.y; dy++){
        EM.Ez(dx, dy) -= lass.EzH(dx, dy) * EM.Hy1d2[dx - 1];
    }

    //return EM;

}


// Checking Absorbing Boundary Conditions (ABC)
void ABCcheck(Field &EM, Loss &lass){

    // defining constant for  ABC
    double c1, c2, c3, temp1, temp2;
    temp1 = sqrt(lass.EzH(0,0) * lass.HyE(0,0));
    temp2 = 1.0 / temp1 + 2.0 + temp1;
    c1 = -(1.0 / temp1 - 2.0 + temp1) / temp2;
    c2 = -2.0 * (temp1 - 1.0 / temp1) / temp2;
    c3 = 4.0 * (temp1 + 1.0 / temp1) / temp2;
    size_t dx, dy;

    // Setting ABC for top
    for (dx = 0; dx < spacex; dx++){
        EM.Ez(dx, spacey - 1) = c1 * (EM.Ez(dx, spacey - 3) + EM.Etop(0, 1, dx))
                      + c2 * (EM.Etop(0, 0, dx) + EM.Etop(2, 0 , dx)
                              -EM.Ez(dx,spacey - 2) -EM.Etop(1, 1, dx))
                      + c3 * EM.Etop(1, 0, dx) - EM.Etop(2, 1, dx);

       // memorizing fields...
        for (dy = 0; dy < 3; dy++){
            EM.Etop(dy, 1, dx) = EM.Etop(dy, 0, dx);
            EM.Etop(dy, 0, dx) = EM.Ez(dx, spacey - 1 - dy);
        }
    }

    // Setting ABC for bottom
    for (dx = 0; dx < spacex; dx++){
        EM.Ez(dx,0) = c1 * (EM.Ez(dx, 2) + EM.Ebot(0, 1, dx))
                      + c2 * (EM.Ebot(0, 0, dx) + EM.Ebot(2, 0 , dx)
                              -EM.Ez(dx,1) -EM.Ebot(1, 1, dx))
                      + c3 * EM.Ebot(1, 0, dx) - EM.Ebot(2, 1, dx);

        // memorizing fields...
        for (dy = 0; dy < 3; dy++){
            EM.Ebot(dy, 1, dx) = EM.Ebot(dy, 0, dx);
            EM.Ebot(dy, 0, dx) = EM.Ez(dx, dy);
        }
    }

    // ABC on right
    for (dy = 0; dy < spacey; dy++){
        EM.Ez(spacex - 1,dy) = c1 * (EM.Ez(spacex - 3,dy) + EM.Eright(0, 1, dy))
                      + c2 * (EM.Eright(0, 0, dy) + EM.Eright(2, 0 , dy)
                              -EM.Ez(spacex - 2,dy) -EM.Eright(1, 1, dy))
                      + c3 * EM.Eright(1, 0, dy) - EM.Eright(2, 1, dy);

        // memorizing fields...
        for (dx = 0; dx < 3; dx++){
            EM.Eright(dx, 1, dy) = EM.Eright(dx, 0, dy);
            EM.Eright(dx, 0, dy) = EM.Ez(spacex - 1 - dx, dy);
        }
    }


    // Setting ABC for left side of grid. Woo!
    for (dy = 0; dy < spacey; dy++){
        EM.Ez(0,dy) = c1 * (EM.Ez(2,dy) + EM.Eleft(0, 1, dy))
                      + c2 * (EM.Eleft(0, 0, dy) + EM.Eleft(2, 0 , dy)
                              -EM.Ez(1,dy) -EM.Eleft(1, 1, dy))
                      + c3 * EM.Eleft(1, 0, dy) - EM.Eleft(2, 1, dy);

        // memorizing fields...
        for (dx = 0; dx < 3; dx++){
            EM.Eleft(dx, 1, dy) = EM.Eleft(dx, 0, dy);
            EM.Eleft(dx, 0, dy) = EM.Ez(dx, dy);
        }
    }

    //return EM;
}


// Adding plane wave
double planewave(int time, int loc, double Cour, int ppw){
    double plane;

    plane = sin((1 / (double)ppw) * (Cour * (double)time -
                 (double)loc));
    //plane = sin((double)(time-loc) * 3.5 / radius);

    return plane;
}

