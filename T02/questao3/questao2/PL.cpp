#include <stdio.h>
#include <time.h>
#include <iostream>
#include <limits.h>
#include <math.h>
#include <ilcplex/ilocplex.h>
#include <vector>
#include <list>
#include "readFile.h"

#define eps 0.000001
#define MAXN 1001

ILOSTLBEGIN
using namespace std;

int numClientes, numFabricas;
int n;
double custos[MAXN][MAXN];
double oferta[MAXN];
double demanda[MAXN];

IloEnv env;
IloModel model(env);
IloIntVarArray var_yo(env);
IloIntVarArray var_yd(env);
IloObjective obj = IloMaximize(env);
IloCplex cplex(env);

void createModel()
{
    IloExpr exp(env);
    IloRangeArray con(env);

    try
    {

        //printf("Creating %d X variables...\n");
        for (int i = 0; i < numClientes; i++)
        {
            char nome[20];
            sprintf(nome, "yd%d", i + 1);
            var_yd.add(IloIntVar(env, 0, IloInfinity));
            var_yd[i].setName(nome);
        }

        for (int j = 0; j < numFabricas; j++)
        {
            char nome[20];
            sprintf(nome, "yo%d", j + 1);
            var_yo.add(IloIntVar(env, 0, IloInfinity));
            var_yo[j].setName(nome);
        }

        exp.clear();
        //printf("Creating Objective Function...\n\n");
        for (int i = 0; i < numClientes; i++)
        {
            obj.setLinearCoef(var_yd[i], -demanda[i]);
        }

        for (int j = 0; j < numFabricas; j++)
        {
            obj.setLinearCoef(var_yo[j], oferta[j]);
        }

        exp.clear();

        //printf("Adding constraints...\n\n");
        for (int i = 0; i < numClientes; i++)
        {
            for (int j = 0; j < numFabricas; j++)
            {
                exp += -var_yd[i] + var_yo[j];
                model.add(exp <= custos[i][j]);
                exp.clear();
            }
        }

        //model.add(con);
        //con.clear();
        model.add(obj);
        cplex.extract(model);
        cplex.exportModel("PL.lp");
    }
    catch (IloException &e)
    {
        cerr << "Concert exception caught: " << e << endl;
    }
    catch (...)
    {
        cerr << "Unknown exception caught" << endl;
    }
}

bool solveMaster(double *x, double *sol_val)
{ // Return true if the master problem is feasible
    // Optimize the problem
    if (!cplex.solve() && cplex.getStatus() != IloAlgorithm::Infeasible)
    {
        env.error() << "Failed to optimize LP" << endl;
        cout << cplex.getStatus() << endl;
        throw(-1);
    }

    // Get solution
    if (cplex.getStatus() != IloAlgorithm::Infeasible)
    {
        *sol_val = cplex.getObjValue();
        //printf("\nSolution value = %.2lf ",*sol_val);
        for (int i = 0; i < numClientes; i++)
                x[i] = cplex.getValue(var_yd[i]);
        for (int j = 0; j < numFabricas; j++)
               x[numClientes + j] = cplex.getValue(var_yo[j]);
        return true;
    }
    return false;
}

void Procedure()
{
    double sol_val = -1;
    double *x = NULL;

    x = (double *)malloc((n) * sizeof(double));
    memset(x, 0, n * sizeof(double));

    if (!solveMaster(x, &sol_val))
        printf("INFEASIBLE!\n"); // Return false if the master problem is infeasible

    printf("Objective Function Value = %.2lf\n", sol_val);

    free(x);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Please, specify the file.\n");
        return 0;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL)
    {
        printf("Could not open %s.\n", argv[1]);
        return 0;
    }

    fscanf(f, "%d %d", &numClientes, &numFabricas);

    n = numClientes * numFabricas;

    for (int i = 0; i < numClientes; i++)
    {
        fscanf(f, "%lf", &demanda[i]);
    }

    for (int i = 0; i < numFabricas; i++)
    {
        fscanf(f, "%lf", &oferta[i]);
    }

    for (int i = 0; i < numClientes; i++)
    {
        for (int j = 0; j < numFabricas; j++)
        {
            fscanf(f, "%lf", &custos[i][j]);
        }
    }

    std::cout << numClientes << " " << numFabricas << std::endl;

    for (int i = 0; i < numClientes; i++)
    {
        for (int j = 0; j < numFabricas; j++)
        {
            std::cout << custos[i][j] << " ";
        }
        std::cout << std::endl;
    }

    fclose(f);

    clock_t start, end;
    double elapsed;
    start = clock();
    // Solving the problem
    cplex.setOut(env.getNullStream());     // Do not print cplex informations
    cplex.setWarning(env.getNullStream()); // Do not print cplex warnings
    createModel();
    Procedure();

    // Free memory

    // Getting run time
    env.end();
    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time: %.5g second(s).\n", elapsed);

    return 0;
}
