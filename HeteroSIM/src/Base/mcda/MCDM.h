#ifndef MCDM_ALGORITHMS_H_INCLUDED
#define MCDM_ALGORITHMS_H_INCLUDED

#include <fstream>
#include <string>
#include <sstream>      // for parsing
#include <algorithm> // max, min functions, etc
#include "MatrixFunctions.h"  //linear algebra functions related to Matrix class
namespace McdaAlg {};
namespace McdaAlg
{

//the structure that keeps all properties of crit for Enh MaxMin procedure
struct Norma
{
    std::string nameCrit;
    bool upwardType {true};
    double u;
    double l;
};



//Normalization stage

// parse string into vector of strings
std::vector<std::string> split(const std::string &input, char delim);

Matrix parseInputString(const std::string &input, char delim, int critNumb);

void showSetNorma(std::vector<Norma> &norm);

//read parameters fro enhMaxMin from file enhNorm.dat
std::vector<Norma> setNorma(int critNumb, std::string path);

std::vector<Norma> setNorma3(int critNumb, std::string path,Matrix a);

std::vector<Norma> selectNormCriteria(std::vector<Norma> norm, Matrix decisionCriteriaIndexes);

Matrix norm1(Matrix a, std::vector<Norma> norm);

// Normalization method 3
Matrix norm3(Matrix a, std::vector<Norma> norm);

//Simple  weighting

Matrix simple_weighting(std::string weights);

//Weighting stage
Matrix wls_weighting(Matrix A);

// objective weighting
Matrix entropy_weighting(Matrix D);

// hybrid weighting
Matrix hybrid_weighting(Matrix w_s, Matrix w_obj, double k);

//find ideal solution for MCDA
Matrix idealSolution(Matrix D);

//find anti-ideal solution for MCDA
Matrix antiIdealSolution(Matrix D);

Matrix SAW(Matrix D, Matrix W);

Matrix GRA(Matrix D, Matrix W);

Matrix TOPSIS(Matrix D, Matrix W);

// random numbers generated in a given range
double random(int min, int max);


/*if normalized matrix has values =1 or =0, it is very likely, that
they are the same for several alternatives (from observations).
The idea is to add or substract some very small random value,
such that ideal and antiIdeal vectors will be different.
*/

void checkAndModifyInputForVikor(Matrix &D);

//VIKOR version w/o stability checking
Matrix VIKOR(Matrix D, Matrix W, double v);

//overloaded VIKOR w stability checking
Matrix VIKOR( Matrix D, Matrix W, bool &checkStability);

// consistency calculation
double calculateConsistency(Matrix A,Matrix w,int n);
//read matrix A from files stream.dat or conv.dat
Matrix readPreferences(std::string trafficType,std::string path, int critNumb);

Matrix selectSomeCriteria(Matrix A, Matrix decisionCriteriaIndexes);

int decisionProcess(std::string decisionData, std::string path,
        std::string weightingMethod,std::string weights, int critNumb, std::string trafficType,
        std::string algName);

// To display some matrix expresssions
void displayExpression(Matrix D, int i, int j);

}

#endif // MCDM_ALGORITHMS_H_INCLUDED
