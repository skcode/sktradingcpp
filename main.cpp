#include <iostream>
#include "logger.hpp"
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "yahoofetch.hpp"
#include "googlefetch.hpp"
#include "starter-minimum_variance_multithreading.hpp"
#include <unordered_map>
#include "temp.hpp"
#include "tradingsystems.hpp"
using namespace std;







/**
USAGE
sktrading {MLSE/NYSE} [param file]
*/

int main(int argc, const char ** argv)

{

    string db="db";

    
    if (argc<=1)
    {
        cout << "\nNo params";
        //security::initialize_db(db);
        minvariance_mt::testmv(db,"MLSE");
        //security::populate_db(db);
        //tradingsystems::checkCorrelation(db);
        //testmv(db,"NYSE");
    }
    else if (string(argv[1]).compare("initializedb")==0) {
        LOG_DEBUG("DB Initialization");
        if (argc!=3) THROW_EXCEPTION("bad params for initializedb");        
        security::initialize_db(argv[2]);
    }
    else if (string(argv[1]).compare("minimumvariancemt")==0)
    {
        LOG_DEBUG("Minimum variance multithreading run");
        if (argc!=3) THROW_EXCEPTION("bad params for minimumvariancemt");                
        //ifstream infile(argv[3]);
        string m=argv[2];
        if (m.compare("MLSE")!=0 && m.compare("NYSE")!=0) THROW_EXCEPTION("bad market "<<argv[2]);
        //if (!infile.good()) THROW_EXCEPTION("wrong filename "<<argv[3]);
        //size_t trainfrom= (size_t)std::stoi(argv[4]);        
        minvariance_mt::testmv(db,string(argv[2]));
    }
    else if (string(argv[1]).compare("currentbestmv")==0) {
        LOG_DEBUG("current best mv run");
        if (argc!=2) THROW_EXCEPTION("bad params for currentbestmv");                
        //ifstream infile(argv[3]);
        //string m=argv[2];
        //if (m.compare("MLSE")!=0 && m.compare("NYSE")!=0) THROW_EXCEPTION("bad market "<<argv[2]);
        //if (!infile.good()) THROW_EXCEPTION("wrong filename "<<argv[3]);
        //size_t trainfrom= (size_t)std::stoi(argv[4]);        
        
        timeseries ts=WalkForwardMinimumVariance::getCurrentBest(WalkForwardMinimumVariance::getSecurities(db,"","EUR","MLSE","STOCK"),85,15);
        ts.toCSV("/tmp/best.csv");
        //minvariance_mt::testmv(db,string(argv[2]));    
    }
    else
    {
        for (int i=1; i<argc; i++) LOG_DEBUG("arg "<<i<<"\t"<<argv[i]);
        try {
            LOG_INFO("usages:");
            LOG_INFO("initializedb <dbfilename>");
            LOG_INFO("minimumvariancemt <MLSE|NYSE> <paramsfilename>");
            
        //security::populate_db_SECINFO(db);
        //vector<string> s=security::getSecList(db);
        //for (auto x: s) cout<<"\n"<<x;

        //WalkForwardMinimumVariance wfmv(WalkForwardMinimumVariance::getSecurities(db));
        //testmv(db,"MLSE");
        //timeseries best=WalkForwardMinimumVariance::getCurrentBest(WalkForwardMinimumVariance::getSecurities(db,"","EUR","MLSE"),252,10);
        //best.toCSV("/tmp/t.csv");
        }
            catch (std::exception &e ) {LOG_DEBUG(e.what());}
        //THROW_EXCEPTION("bad arguments");
    }



    return 0;
}
