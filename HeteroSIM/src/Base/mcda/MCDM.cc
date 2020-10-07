#include "../mcda/MCDM.h"
#include<boost/lexical_cast.hpp>
#include <string>

namespace McdaAlg {};
namespace McdaAlg
{
// parse string into vector of strings
std::vector<std::string> split(const std::string &input, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

Matrix parseInputString(const std::string &input, char delim, int critNumb)
{
    std::vector<std::string> out;
    out=split(input,delim);                 //parse to vector of strings
    int lengthOfString = out.size();
    int numberOfAlternatives = lengthOfString/critNumb;     //define number of alternatives (always int)
    Matrix outNumb(numberOfAlternatives,critNumb);               // init matrix
    for (int j=0; j<numberOfAlternatives; j++)
    {
        for (int i=0; i<critNumb; i++)
        {
            std::istringstream istr(out[i+j*critNumb]);
            std::string temp;
            istr>>temp;
            if (temp=="-inf")   //handle case when there is AP with -inf RSS value
                outNumb.at(j,i) = -100;
            else
                outNumb.at(j,i) = std::stod(temp);
        }
    }
    return outNumb;
}

void showSetNorma(std::vector<Norma> &norm)
{
    int numberOfCrit = norm.size();
    std::cout<<"size of norm structure is : "<<numberOfCrit<<"\n";
    for (int i=0; i<numberOfCrit; i++)
    {
        std::cout<<"The name of the criteria is \t"<<norm[i].nameCrit<<"\n";
        std::cout<<"Is the type of the criteria upward? \t"<<norm[i].upwardType<<"\n";
        std::cout<<"The upper bound is \t"<<norm[i].u<<"\n";
        std::cout<<"The lower bound is \t"<<norm[i].l<<"\n"<<"-------\n";
    }
}

//read parameters fro enhMaxMin from file enhNorm.dat
std::vector<Norma> setNorma(int critNumb, std::string path)
{
    std::vector<Norma> norm;
    char resolved_path[PATH_MAX];
    realpath(path.c_str(), resolved_path);
    std::ifstream inf(std::string(resolved_path)+"/"+"enhNorm"+std::to_string(critNumb)+".dat");
    // If we couldn't open the output file stream for reading
    if (!inf)
    {
        // Print an error and exit
        std::cerr << "Uh oh, file with normalization values could not be opened for reading!" <<"\n";
        exit(1);
    }
    // While there's still stuff left to read
    while (inf)
    {
        // read stuff from the file into matrix A elements
        for (int i=0; i<critNumb; i++)
        {
            //Push back new subject created with default constructor.
            norm.push_back(Norma());
            //Vector now has 1 element @ index 0, so modify it.
            inf>>norm[i].nameCrit;
            inf>>norm[i].upwardType;
            inf>>norm[i].u;
            inf>>norm[i].l;
        }
    }
    norm.resize(critNumb); //assure proper size
    return norm;
}

std::vector<Norma> selectNormCriteria(std::vector<Norma> norm, Matrix decisionCriteriaIndexes)
{
    std::vector<Norma> normCut;
    int numbOfCutCriteria=decisionCriteriaIndexes.size(1);
    for (int i=0; i<numbOfCutCriteria; i++)
    {
        //Push back new subject created with default constructor.
        normCut.push_back(Norma());
        //Vector now has 1 element @ index 0, so modify it.
        normCut[i].nameCrit = norm[decisionCriteriaIndexes.at(i,0)].nameCrit;
        normCut[i].upwardType = norm[decisionCriteriaIndexes.at(i,0)].upwardType;
        normCut[i].u = norm[decisionCriteriaIndexes.at(i,0)].u;
        normCut[i].l = norm[decisionCriteriaIndexes.at(i,0)].l;
    }
    normCut.resize(numbOfCutCriteria); //assure proper size
    return normCut;
}


Matrix norm1(Matrix a, std::vector<Norma> norm)
{
    int m = a.size(1);
    int n = a.size(2);
    Matrix d(m,n);
    for (int j=0; j<n; j++)
    {
        if (norm[j].upwardType == true)
        {
            for (int i=0; i<m; i++)
            {
                d.at(i,j) = (a.at(i,j) - norm[j].l) / (norm[j].u-norm[j].l);
                //std::cout<<d.at(i,j)<<"\n";
            }
        }
        else
        {
            for (int i=0; i<m; i++)
            {
                d.at(i,j) = (norm[j].u - a.at(i,j)) / (norm[j].u-norm[j].l);
                //std::cout<<d.at(i,j)<<"\n";
            }
        }
    }
    return d;
}


//read parameters fro enhMaxMin from file enhNorm.dat
std::vector<Norma> setNorma3(int critNumb, std::string path,Matrix a)
{
    std::vector<Norma> norm;
    char resolved_path[PATH_MAX];
    realpath(path.c_str(), resolved_path);
    std::ifstream inf(std::string(resolved_path)+"/"+"enhNorm"+std::to_string(critNumb)+".dat");
    std::cout<< "absolute path is= " << resolved_path <<std::endl;
    // If we couldn't open the output file stream for reading
    if (!inf)
    {
        // Print an error and exit
        std::cerr << "Uh oh, file with normalization values could not be opened for reading!" <<"\n";
        exit(1);
    }
    // While there's still stuff left to read
    while (inf)
    {
        // read stuff from the file into matrix A elements
        for (int i=0; i<critNumb; i++)
        {
            //Push back new subject created with default constructor.
            norm.push_back(Norma());
            //Vector now has 1 element @ index 0, so modify it.
            inf>>norm[i].nameCrit;
            inf>>norm[i].upwardType;
            if(norm[i].upwardType==1)
            {
                norm[i].upwardType=true;
            }else
            {
                norm[i].upwardType=false;
            }
            norm[i].u=maxElement(a,i,"column");
            norm[i].l=minElement(a,i,"column");
        }
    }
    norm.resize(critNumb); //assure proper size
    return norm;
}

Matrix norm3(Matrix a, std::vector<Norma> norm)
{
    int m = a.size(1);
    int n = a.size(2);
    Matrix d(m,n);
    for (int j=0; j<n; j++)
    {
        if (norm[j].upwardType == 1)
        {
            for (int i=0; i<m; i++)
            {
                d.at(i,j) = a.at(i,j) / (norm[j].u);
                //std::cout<<d.at(i,j)<<"\n";
            }
        }
        else
        {
            for (int i=0; i<m; i++)
            {
                d.at(i,j) = (norm[j].l) / (a.at(i,j));
                //std::cout<<d.at(i,j)<<"\n";
            }
        }
    }
    return d;
}


Matrix readPreferences(std::string trafficType,std::string path, int critNumb)
{

    Matrix A(critNumb,critNumb);
    char resolved_path[PATH_MAX];
    realpath(path.c_str(), resolved_path);
    std::ifstream inf(std::string(resolved_path)+"/"+"A"+std::to_string(critNumb)+trafficType+".dat");
    // If we couldn't open the output file stream for reading
    if (!inf)
    {
        // Print an error and exit
        std::cerr << "Uh oh, file with preferences could not be opened for reading!" <<"\n";
        exit(1);
    }
    // While there's still stuff left to read
    while (inf)
    {
        // read stuff from the file into matrix A elements
        for (int i=0; i<critNumb; i++)
        {
            for (int j=0; j<critNumb; j++)
            {
                inf>>A.at(i,j);
                //std::cout<<A.at(i,j)<<"\n";
            }
        }
    }
    return A;
}



void displayExpression(Matrix D, int i, int j)
{
std::cout<< "D("+ std::to_string(i) +","+ std::to_string(j)+")*log(D("+std::to_string(i) +","+ std::to_string(j)+"))= "
<< (D.at(i,j)*std::log(D.at(i,j))) <<"\n";
}
//
// TODO (mouna1#1#): Correct entropy ...
//
Matrix entropy_weighting(Matrix D)
{

    double alterNum=D.size(1);
    int n=int(alterNum);
    Matrix W(n,1);
    Matrix H(n,1);
    Matrix r(n,D.size(2));

    for (int i=0; i<n; i++)
    {
    for(int j=0; j<D.size(2); j++)
    {
    r.at(j,i)=D.at(j,i)/sum(D,i,"column");
    }
    }

    std::cout<<"r= \n" ;
    r.print();

    double h=-1/std::log(alterNum);
    for (int i=0; i<n; i++)
    {
    for(int j=0; j<H.size(2); j++)
    {

    H.at(i,j)=(h*entropicSum(r,i));

    }
    }
    std::cout << "entropy " <<"\n";
    H.print();
    double sh=0;


    for(int i=0; i<n; i++)
    {
    for(int j=0; j<W.size(2); j++)
    {
         W.at(i,j)=(1-H.at(i,j))/(alterNum-sum(H,0,"column"));

    }
    }
    return W;
}





Matrix hybrid_weighting(Matrix w_s, Matrix w_obj, double k)
{

    Matrix W(w_s.size(1),1);

    for (int i=0; i<W.size(1);i++)
    {
    for(int j=0; j<W.size(2);j++)
    {
     W.at(i,j)=k*w_s.at(i,j)+(1-k)*w_obj.at(i,j);
    }
    }
    return W;


}


Matrix wls_weighting(Matrix A)
{
    int critNumb = A.size(1);
    Matrix e(critNumb,1);

    e = ones(critNumb,1);
    Matrix B(critNumb,critNumb);

    B = diag(transpose(A)*A) - A - transpose(A) + critNumb*eye(critNumb);
    //std::cout<<"B matrix: \n";
    //B.print();

    Matrix w(critNumb,1);
    w = inv(B)* e / (transpose(e)*inv(B)*e);
    return w;
}




//find ideal solution for MCDA
Matrix idealSolution(Matrix D)
{
    int critNumb=D.size(2);
    Matrix ideal(1,critNumb);
    Matrix idealIndex(1,critNumb);
    for (int i=0; i<critNumb; i++)
    {
        ideal.at(0,i) = D.at(maxIndex(D,i,"column"),i);
    }
    return ideal;
}

//find anti-ideal solution for MCDA
Matrix antiIdealSolution(Matrix D)
{
    int critNumb=D.size(2);
    Matrix antiIdeal(1,critNumb);
    Matrix antiIdealIndex(1,critNumb);
    for (int i=0; i<critNumb; i++)
    {
        antiIdeal.at(0,i) = D.at(minIndex(D,i,"column"),i);
    }
    return antiIdeal;
}

Matrix SAW(Matrix D, Matrix W)
{
    Matrix score(D.size(1),W.size(2));
    score = D * W;
    return score;
}

Matrix GRA(Matrix D, Matrix W)
{
    int altNumb=D.size(1);
    int critNumb=D.size(2);
    Matrix score(D.size(1),1);
    Matrix ideal = idealSolution(D);
    for (int i=0; i<altNumb; i++)
    {
        double stuff {0};
        for (int j=0; j<critNumb; j++)
        {
            stuff += W.at(j,0) * fabs(D.at(i,j)-ideal.at(0,j));
        }
        score.at(i,0) = 1 / (stuff + 1);
    }
    return score;
}

Matrix TOPSIS(Matrix D, Matrix W)
{
    int altNumb=D.size(1);
    int critNumb=D.size(2);
    Matrix V(altNumb,critNumb);
    for (int i=0; i<altNumb; i++)
    {
        for (int j=0; j<critNumb; j++)
        {
            V.at(i,j) = D.at(i,j) * W.at(j,0);
        }
    }
    Matrix Vplus = idealSolution(V);        //find ideal solution
    Matrix Vminus = antiIdealSolution(V);    // anti-ideal
    Matrix Splus(altNumb,1);
    Matrix Sminus(altNumb,1);
    Matrix RC(altNumb,1);
    for (int i=0; i<altNumb; i++)
    {
        for (int j=0; j<critNumb; j++)
        {
            Splus.at(i,0) += pow((Vplus.at(0,j) - V.at(i,j)),2);
            Sminus.at(i,0) += pow((V.at(i,j) - Vminus.at(0,j) ),2);
        }
        Splus.at(i,0) = pow(Splus.at(i,0), 0.5);
        Sminus.at(i,0) = pow(Sminus.at(i,0), 0.5);
        RC.at(i,0) = Sminus.at(i,0) / (Splus.at(i,0) + Sminus.at(i,0));
    }
    return RC;
}

/*if normalized matrix has values =1 or =0, it is very likely, that
they are the same for several alternatives (from observations).
The idea is to add or substract some very small random value,
such that ideal and antiIdeal vectors will be different.
*/
void checkAndModifyInputForVikor(Matrix &D)
{
    int altNumb=D.size(1);
    int critNumb=D.size(2);
    for (int i=0; i<altNumb; i++)
    {
        for (int j=0; j<critNumb; j++)
        {
            if (D.at(i,j)==0)
            {
                D.at(i,j) = D.at(i,j) + random(0,90)/10000;
            }
            else if(D.at(i,j)==1)
            {
                D.at(i,j) = D.at(i,j) - random(0,90)/10000;
            }
        }
    }
}

// random numbers generated in a given range
double random(int min, int max)
{
    static const double fraction = 1.0 / (static_cast<double>(RAND_MAX) + 1.0);  // static used for efficiency, so we only calculate this value once
    return static_cast<int>(rand() * fraction * (max - min + 1) + min);
}



//VIKOR version w/o stability checking
Matrix VIKOR(Matrix D, Matrix W,  double v)
{
    int altNumb=D.size(1);
    int critNumb=D.size(2);
    checkAndModifyInputForVikor(D);         //in case of identical values
    Matrix ideal = idealSolution(D);        //find ideal solution
    Matrix antiIdeal = antiIdealSolution(D);    // anti-ideal


    std::cout<<"\n Ideal solution:";
    ideal.print();
    std::cout<<"\n Anti-ideal solution:";
    antiIdeal.print();

    Matrix S(altNumb,1);
    Matrix R(altNumb,1);
    for (int i=0; i<altNumb; i++)
    {
        Matrix stuff(1,critNumb);
        for (int j=0; j<critNumb; j++)
        {
            stuff.at(0,j) = W.at(j,0) * (ideal.at(0,j) - D.at(i,j)) / (ideal.at(0,j) - antiIdeal.at(0,j));
        }
        R.at(i,0) = maxElement(stuff,0,"row");

        S.at(i,0) = sum(stuff,0,"row");


    }
    std::cout<< "R= ";
    R.print();
    std::cout<< "S= " ;
    S.print();

    double Rplus =  maxElement(R,0,"column");
    //std::cout<<"R+ "<<Rplus<<"\n";
    double Rminus =  minElement(R,0,"column");
    //std::cout<<"R- "<<Rminus<<"\n";
    double Splus =  maxElement(S,0,"column");
    //std::cout<<"S+ "<<Splus<<"\n";
    double Sminus =  minElement(S,0,"column");
    //std::cout<<"S- "<<Sminus<<"\n";

    //double v {0.5};     // 0.5 corresponds to consensus strategy
    Matrix Q(altNumb,1);
    for (int i=0; i<altNumb; i++)
    {
        Q.at(i,0)=v*(S.at(i,0)-Sminus)/(Splus-Sminus) +
                  (1-v)*(R.at(i,0)-Rminus)/(Rplus-Rminus);
        //std::cout<<(1-v)*(R.at(i,0)-Rminus)/(Rplus-Rminus) <<"\n";
    }
    //std::cout<<" Q marix of VIKOR= \n";
    //Q.print();
    return Q;
}
//overloaded VIKOR w stability checking
Matrix VIKOR( Matrix D, Matrix W, bool &checkStability)
{
    int altNumb=D.size(1);
    int critNumb=D.size(2);
    checkAndModifyInputForVikor(D);         //in case of identical values
    Matrix ideal = idealSolution(D);        //find ideal solution
    Matrix antiIdeal = antiIdealSolution(D);    // anti-ideal
    Matrix S(altNumb,1);
    Matrix R(altNumb,1);
    for (int i=0; i<altNumb; i++)
    {
        Matrix stuff(1,critNumb);
        for (int j=0; j<critNumb; j++)
        {
            stuff.at(0,j) = W.at(j,0) * (ideal.at(0,j) - D.at(i,j)) / (ideal.at(0,j) - antiIdeal.at(0,j));
        }
        R.at(i,0) = maxElement(stuff,0,"row");
        S.at(i,0) = sum(stuff,0,"row");
    }

    double Rplus =  maxElement(R,0,"column");
    double Rminus =  minElement(R,0,"column");

    double Splus =  maxElement(S,0,"column");
    double Sminus =  minElement(S,0,"column");

    double v {0.4};     // corresponds to consensus strategy
    Matrix Q(altNumb,1);
    for (int i=0; i<altNumb; i++)
    {
        Q.at(i,0)=v*(S.at(i,0)-Sminus)/(Splus-Sminus) +
                  (1-v)*(R.at(i,0)-Rminus)/(Rplus-Rminus);
        //std::cout<<(1-v)*(R.at(i,0)-Rminus)/(Rplus-Rminus) <<"\n";
    }
    //check conditions of acceptable advantage and stability
    if ((maxIndex(Q,0,"column") == maxIndex(S,0,"column") &&
            maxIndex(Q,0,"column") == maxIndex(R,0,"column")) &&
            (  (maxIndex(Q,0,"column")-secondMaxIndex(Q,0,"column")) >= (1/(altNumb-1)) )    )
    {
        checkStability = true;
        std::cout<<"\n Decision is stable \n";
        return Q;
    }
    else
    {
        checkStability = false;
        std::cout<<"\n Decision is NOT stable \n";
        return Q;
    }
}



Matrix selectSomeCriteria(Matrix A, Matrix decisionCriteriaIndexes)
{
    int cutCritNumb=decisionCriteriaIndexes.size(1);
    int altNumb = A.size(2);
    Matrix Acut(altNumb,cutCritNumb);
    for(int i=0; i<altNumb; ++i)
    {
        for(int j=0; j<cutCritNumb; ++j)
        {
            Acut.at(i,j)=A.at(i,decisionCriteriaIndexes.at(j,0));
        }
    }
    return Acut;
}

int decisionProcess(std::string allPathsCriteriaValues,std::string path,int critNumb,std::string trafficType,std::string algName)
{

    Matrix C = parseInputString(allPathsCriteriaValues,',',critNumb);
    std::cout<<"Criteria matrix : " <<"\n";
    C.print();
    std::vector<Norma> norm = setNorma3(critNumb,path,C); //set norm parameters

    std::cout<<"Normalized matrix : " <<"\n";

    // Normalization stage ...
    Matrix D = norm3(C,norm);
    D.print();
    // weighting stage ...
    Matrix A = readPreferences(trafficType,path,critNumb);
    Matrix W = wls_weighting(A);
    std::cout<<"Subjective Weighted matrix : " <<"\n";
    W.print();

//    Matrix W_obj= entropy_weighting(D);
//
//    std::cout<<"Objective Weighted matrix : " <<"\n";
//    W_obj.print();
//
//    Matrix W=hybrid_weighting(W_s,W_obj,0.9);
//    std::cout<<"Hybrid Weighted matrix : " <<"\n";
//    W.print();
    // decision stage ...

    Matrix score(D.size(1),1);

    if (algName == "SAW")
        score = SAW(D,W);
    else if (algName == "GRA")
        score = GRA(D,W);
    else if (algName == "TOPSIS")
        score = TOPSIS(D,W);
    else if (algName == "VIKOR")
        score = VIKOR(D,W,0.5); // v=0.5, strategy with consensus.....
    else if(algName=="VIKOR_STABILITY_CHECK")
        score = VIKOR(D,W,true);
    else
        std::cerr<<"Wrong entered name!!\n";

    std::cout<<"Score with " << algName << ": "<<"\n";
    score.print();

    int bestIndexFromGood=0;
    if (algName=="VIKOR")
        bestIndexFromGood = minIndex(score,0,"column");
    else
        bestIndexFromGood = maxIndex(score,0,"column");
    return bestIndexFromGood;
}



std::string buildAllPathThreeCriteria(std::vector<double> datarate,std::vector<double> delay,std::vector<double> Th)
{

	std::string allPathsCriteriaValues = "";
	std::string pathsCriteriaValues = "";
	std::vector<std::string> criteriaStr;
    std::string critValuesPerPathStr = "";
    // rssi,  delay, jitter , throughput, cost should have the same length
    int s=delay.size();
    for(int i=0; i<s; i++)
    {

    criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        datarate.at(i)));

	criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        delay.at(i)));
    criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        Th.at(i)));
    }

	for (int a = 0; a < criteriaStr.size(); ++a) {
		if (a == 0) {
			pathsCriteriaValues = pathsCriteriaValues + criteriaStr[a];
		} else {
			pathsCriteriaValues = pathsCriteriaValues + ","
					+ criteriaStr[a];
		}}

	allPathsCriteriaValues = allPathsCriteriaValues + pathsCriteriaValues
			+ ",";
	criteriaStr.clear();

    //std::cout<<"All paths criteria values:  "<<allPathsCriteriaValues<<"\n";

	return allPathsCriteriaValues;
}




std::string buildAllPathFiveCriteria(std::vector<double> rssi,std::vector<double> delay,std::vector<double> jitter
,std::vector<double> th, std::vector<double> cost)
{

	std::string allPathsCriteriaValues = "";
	std::string pathsCriteriaValues = "";
	std::vector<std::string> criteriaStr;
    std::string critValuesPerPathStr = "";
    // rssi,  delay, jitter , throughput, cost should have the same length
    int s=delay.size();
    for(int i=0; i<s; i++)
    {

    criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        rssi.at(i)));

	criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        delay.at(i)));
    criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        jitter.at(i)));

    criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        th.at(i)));

    criteriaStr.push_back(
	                boost::lexical_cast<std::string>(
	                        cost.at(i)));
    }

	for (int a = 0; a < criteriaStr.size(); ++a) {
		if (a == 0) {
			pathsCriteriaValues = pathsCriteriaValues + criteriaStr[a];
		} else {
			pathsCriteriaValues = pathsCriteriaValues + ","
					+ criteriaStr[a];
		}}

	allPathsCriteriaValues = allPathsCriteriaValues + pathsCriteriaValues
			+ ",";
	criteriaStr.clear();

    //std::cout<<"All paths criteria values:  "<<allPathsCriteriaValues<<"\n";

	return allPathsCriteriaValues;
}




//
double calculateConsistency(Matrix A,Matrix w,int n)
{

    Matrix s(1,n);
    for(int i=0; i<n; i++)
    {
    s.at(0,i)=sum(A,i,"column");
    }
    //s.print();
    Matrix sp(1,n);
    sp=s*w;
    //sp.print();
    //double lambdaMax=sum(s*w,0, "row");
    double CI=(sp.at(0,0)-n) /(n-1);
    return CI/0.58;
}


}

