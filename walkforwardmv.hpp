#ifndef WALKFORWARDMV_HPP_INCLUDED
#define WALKFORWARDMV_HPP_INCLUDED
#include <cstdio>
#include <iostream>
#include <map>
#include <fstream>
#include "security.h"
#include "logger.hpp"
#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <fstream>
#include "security.h"
#include "logger.hpp"
#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <bitset>
#include <random>
#include <thread>
#include <unordered_map>


using namespace std;

class WalkForwardMinimumVariance
{
private:



    const timeseries aller;
    //PARAMETERS
    size_t min_samples; //2500 default
    time_t max_seconds_gap;//60*60*24*7  default
    time_t max_seconds_before_now;//60*60*24*7  default
    double max_diff_values_pct;//20 pct 
    double min_mean_liquidity;//5000 for italy, 100000 for usa
    size_t min_train_serie;//25 for NYSE, 15 for MLSE
    size_t fixed_test_serie_random_opt; //30 seems the best for usa, 15 for italy
    size_t cycles_random_opt;//at least 1000000
    double sharpe_treshold;//0 best
    double transaction_penalty_pc;
    double max_tests_variance; //.6 seems the best
    //

    void setDefault()
    {
        min_samples=2500; //2500 default
        max_seconds_gap=60*60*24*7;//60*60*24*7  default
        max_seconds_before_now=60*60*24*7;//60*60*24*7  default
        max_diff_values_pct=20.0;//default
        min_mean_liquidity=100000;//5000 for italy, 100000 for usa
        min_train_serie=15;//25 for NYSE, 15 for MLSE
        fixed_test_serie_random_opt=15; //30 seems the best for usa, 15 for italy
        cycles_random_opt=500000;//at least 1000000
        sharpe_treshold=0.00;//0 best
        transaction_penalty_pc=0.005;
        max_tests_variance=.6; //.6 seems the best
    }

    void read_params_from_file(string filename)
    {
        LOG_DEBUG(filename);
        ifstream inputFile(filename);
        string line;
        while (getline(inputFile, line))
        {
            LOG_DEBUG(line)
            istringstream ss(line);

            if (line.find("min_samples")!=string::npos) {
                LOG_DEBUG( line.substr(line.find("=")+1,line.size()-line.find("=")) );
                this->set_min_samples(std::stoi(line.substr(line.find("=")+1,line.size()-line.find("="))));
            }

            if (line.find("max_seconds_gap")!=string::npos) {
                this->set_max_seconds_gap(std::stol(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }
            if (line.find("max_seconds_before_now")!=string::npos) {
                this->set_max_seconds_before_now(std::stol(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }
            if (line.find("max_diff_values_pct")!=string::npos) {
                this->set_max_diff_values_pct(std::stod(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }
            if (line.find("min_mean_liquidity")!=string::npos) {
                this->set_min_mean_liquidity(std::stod(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }
            if (line.find("min_train_serie")!=string::npos) {
                this->set_min_train_serie(std::stoi(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }

            if (line.find("fixed_test_serie_random_opt")!=string::npos) {
                this->set_fixed_test_serie_random_opt(std::stoi(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }

            if (line.find("cycles_random_opt")!=string::npos) {
                this->set_cycles_random_opt(std::stol(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }
            if (line.find("sharpe_treshold")!=string::npos) {
                this->set_sharpe_treshold(std::stod(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }

            if (line.find("transaction_penalty_pc")!=string::npos) {
                this->set_transaction_penalty_pc(std::stod(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }

            if (line.find("max_tests_variance")!=string::npos) {
                this->set_max_tests_variance(std::stod(line.substr(line.find("=")+1,line.size()-line.find("="))));                //cout << "\n"<<line.substr(0,line.find("="))<< "\t"<<;
            }


        }
        LOG_DEBUG("READ PARAMS FROM "<<filename);
        LOG_DEBUG(this->get_min_samples()<<"\t"<<this->get_max_seconds_gap()<<"\t"<<this->get_max_seconds_before_now()<<"\t"<<this->get_min_mean_liquidity()<<"\t"<<this->get_max_diff_values_pct() );
        LOG_DEBUG(this->get_min_train_serie()<<"\t"<<this->get_fixed_test_serie_random_opt()<<"\t"<<this->get_cycles_random_opt()<<"\t"<<this->get_sharpe_treshold())
        LOG_DEBUG(this->get_transaction_penalty_pc()<<"\t"<<this->get_max_tests_variance());

    }





    const timeseries  init(const vector<security>& securities,string params_file)
    {
        if (params_file.compare("")==0)this->setDefault(); else this->read_params_from_file(params_file);
        timeseries all;
        time_t t0 = time(0);   // get time now
        for (const security &s:securities)
        {
            LOG_DEBUG("symbol "<<s.getName());
            LOG_DEBUG("length "<<s.ts.getLength());

            if (s.ts.getLength()< (min_samples))
            {
                LOG_DEBUG("out for len<min_samples: "<<s.ts.getLength()<<"<"<<min_samples);
                continue;
            }
            LOG_DEBUG("dategap "<<s.ts.getDateGapInSeconds());
            if (s.ts.getDateGapInSeconds()>max_seconds_gap )
            {
                LOG_DEBUG("out for getDateGapInSeconds>max_seconds_gap : "<<s.ts.getDateGapInSeconds()<<">"<<max_seconds_gap );
                continue;
            }
            LOG_DEBUG("lastdate "<<timeseries::date2string(s.ts.getLastDate()))
            if (s.ts.getLastDate()<(t0-max_seconds_before_now) )
            {
                LOG_DEBUG("out for s.ts.getLastDate<(t0-max_seconds_before_now) : "<<s.ts.getLastDate()<<"<"<<(t0-max_seconds_before_now));
                continue;
            }
            double volmean=0;
            for (size_t k=0; k<s.ts.getLength(); k++) volmean+=s.close(k)*s.volume(k);
            volmean/=s.ts.getLength();
            if (volmean<min_mean_liquidity)
            {
                LOG_DEBUG("out for volmean<min_mean_liquidity : "<<volmean<<"<"<<min_mean_liquidity);
                continue;
            }
            bool b_max_diff_values_pct=true;
            for (size_t k=1; k<s.ts.getLength(); k++) 
                if (  (100*(s.close(k)-s.close(k-1) )/s.close(k-1))>this->max_diff_values_pct ) {                    
                    
                    b_max_diff_values_pct=false;
                    LOG_DEBUG("out for diffpct>max_diff_values_pct ");                    
                    break;
                }
            if (!b_max_diff_values_pct) continue;
            try
            {
                s.ts.getserie(3).er();
            }
            catch (std::exception &e)
            {
                {
                    LOG_DEBUG("out for er computation error:"<<e.what());
                    continue;
                }
                continue;
            }
            if (all.getSeriesLen()==0) all=s.ts.getserie(3);//close
            else all=all.merge(s.ts.getserie(3));
        }

        if (all.getLength()<2) THROW_EXCEPTION("too few data , no series matching critetia");
        return all.er();
    }


public:

    size_t get_min_samples() const
    {
        return min_samples;
    }
    time_t get_max_seconds_gap() const
    {
        return max_seconds_gap;
    }
    time_t get_max_seconds_before_now() const
    {
        return max_seconds_before_now;
    }
    
    double get_max_diff_values_pct() const
    {
        return max_diff_values_pct;
    }

    double get_min_mean_liquidity() const
    {
        return min_mean_liquidity;
    }
    size_t get_min_train_serie() const
    {
        return min_train_serie;
    }
    size_t get_fixed_test_serie_random_opt() const
    {
        return fixed_test_serie_random_opt;
    }
    size_t get_cycles_random_opt() const
    {
        return cycles_random_opt;
    }
    double get_sharpe_treshold() const
    {
        return sharpe_treshold;
    }
    double get_transaction_penalty_pc() const
    {
        return transaction_penalty_pc;
    }
    double get_max_tests_variance() const
    {
        return max_tests_variance;
    }

    void set_min_samples(size_t v)
    {
        min_samples=v;
    }
    void set_max_seconds_gap(time_t v)
    {
        max_seconds_gap=v;
    }
    void set_max_seconds_before_now(time_t v)
    {
        max_seconds_before_now=v;
    }
    
    void set_min_mean_liquidity(double v)
    {
        min_mean_liquidity=v;
    }
    void set_min_train_serie(size_t v )
    {
        min_train_serie=v;
    }
    void set_fixed_test_serie_random_opt(size_t v)
    {
        fixed_test_serie_random_opt=v;
    }
    void set_cycles_random_opt(size_t v)
    {
        cycles_random_opt=v;
    }
    void set_sharpe_treshold(double v)
    {
        sharpe_treshold=v;
    }
    void set_transaction_penalty_pc(double v)
    {
        transaction_penalty_pc=v;
    }
    void set_max_tests_variance(double v)
    {
        max_tests_variance=v;
    }
    void set_max_diff_values_pct(double v)
    {
        max_diff_values_pct=v;
    }


    struct statswfmv
    {
        int train_window,test_window;
        double final_equity,maxdd_pc,inmarket_pc;
        //string sector=this.sector,currency=this.currency,market=this.market,type=;
        timeseries equity;
        string log;
    } retstats;

    //const static variable

    static vector<security> getSecurities(string db,string sector ="",string currency="EUR",string market="MLSE",string type="STOCK")
    {

        vector<string> v_seclist=security::getSecList(db,sector,currency,market,type);
        vector<security> securities;
        for (string s: v_seclist)
        {
            try
            {
                securities.push_back(security(db,s,market,timeseries::freq_enum::daily));
                LOG_DEBUG("loading "<<s);
            }
            catch (...) {}
        }
        return securities;
    }

    static timeseries getCurrentBest(const vector<security> & securities,size_t datalen,size_t serielen) {
        timeseries aller;
        time_t t0 = time(0);   // get time now
        size_t cycles_random_opt=10000000;
        double min_mean_liquidity=1000000;
        size_t min_samples=datalen*2,max_seconds_gap=60*60*24*7,max_seconds_before_now=60*60*24*14;
        size_t fixed_test_serie_random_opt=serielen;
        if (datalen<20) THROW_EXCEPTION("data len too short: "<< datalen);
        if (serielen<5) THROW_EXCEPTION("serie len too short: "<< serielen);
        LOG_DEBUG("reading symbols");
        for (const security &s :securities)
        {
            LOG_DEBUG("symbol "<<s.getName());
            LOG_DEBUG("length "<<s.ts.getLength());


            if (s.ts.getLastDate()<(t0-(time_t) max_seconds_before_now) )
            {
                LOG_DEBUG("out for s.ts.getLastDate<(t0-max_seconds_before_now) : "<<s.ts.getLastDate()<<"<"<<(t0-max_seconds_before_now));
                continue;
            }

            if (s.ts.getLength()< (min_samples))
            {
                LOG_DEBUG("out for len<min_samples: "<<s.ts.getLength()<<"<"<<min_samples);
                continue;
            }
            timeseries tempts=s.ts.head(min_samples);
            LOG_DEBUG("dategap "<<tempts.getDateGapInSeconds());
            if (tempts.getDateGapInSeconds()>max_seconds_gap )
            {
                LOG_DEBUG("out for getDateGapInSeconds>max_seconds_gap : "<<tempts.getDateGapInSeconds()<<">"<<max_seconds_gap );
                continue;
            }
            LOG_DEBUG("lastdate "<<timeseries::date2string(tempts.getLastDate()))

            double volmean=0;
            for (size_t k=0; k<tempts.getLength(); k++) volmean+=tempts(k,3)*tempts(k,4);//.close(k)*s.volume(k);
            volmean/=tempts.getLength();
            if (volmean<min_mean_liquidity)
            {
                LOG_DEBUG("out for volmean<min_mean_liquidity : "<<volmean<<"<"<<min_mean_liquidity);
                continue;
            }

            try
            {
                tempts=tempts.getserie(3).er();
            }
            catch (std::exception &e)
            {
                {
                    LOG_DEBUG("out for er computation error:"<<e.what());
                    continue;
                }
                continue;
            }
            if (tempts.sharpe(datalen-1).getLastValue(0)<=0) {
                LOG_DEBUG("sharpe <=0");
                continue;
            }
            if (aller.getSeriesLen()==0) aller=tempts;
            else aller=aller.merge(tempts);
        }
        //for (size_t i=0;i<aller.getSeriesLen();i++) {
        //    LOG_DEBUG("added " << aller.getName(i))
        //}
        LOG_DEBUG("ALLER info--> length : "<<aller.getLength()<<" no. series : "<<aller.getSeriesLen()<<  " first date : "<<aller.getFirstDate()<<" last date : "<<aller.getLastDate());
        if (aller.getLength()<(datalen)) THROW_EXCEPTION("too few data , no series matching critetia");
        if (aller.getSeriesLen()<(serielen)) THROW_EXCEPTION("too few series : "<<aller.getSeriesLen());
        aller=aller.head(datalen);

        timeseries sh=aller.sharpe(datalen-1);
        for (size_t i1=0;i1<sh.getSeriesLen();i1++) LOG_DEBUG(sh.getName(i1)<<"\t"<<sh.getLastValue(i1));
        Eigen::MatrixXd cov = aller.cov();
        for (size_t i1=0;i1<aller.getSeriesLen();i1++) LOG_DEBUG(aller.getName(i1)<<"\t"<<cov(i1,i1));
        LOG_DEBUG("ALLER "<<aller.getLength()<<"\t"<<aller.getSeriesLen());
        Eigen::MatrixXd mean=aller.mean();
        //timeseries tests, teststemp;
        //double ws = 0;

        //random engine
        std::default_random_engine generator(time(0));
        std::vector<size_t> vrandom;
        std::uniform_int_distribution<int> distribution(0, aller.getSeriesLen() - 1);
        auto dice = std::bind(distribution, generator);
        vector<size_t> best_random_opt;


        Eigen::MatrixXd vec_ran = Eigen::MatrixXd::Zero((cov).rows(), 1);
        double current_res = 0;
        size_t cicle_idx = 0;
        for (size_t i = 0; i < cycles_random_opt; i++) {
            vrandom.clear();
            while (vrandom.size() < fixed_test_serie_random_opt) {
                size_t t = dice();
                if (vrandom.empty()) {
                    vrandom.push_back(t);
                    continue;
                }
                if (std::find(vrandom.begin(), vrandom.end(), t) != vrandom.end()) continue;
                vrandom.push_back(t);
            }
            vec_ran.fill(0);
            double wran = 1.0 / fixed_test_serie_random_opt;
            double wran2 = wran*wran;
            for (size_t j = 0; j < fixed_test_serie_random_opt; j++) vec_ran(vrandom.at(j), 0) = wran;

            //double res=(vec_ran.transpose()*(*cov)*vec_ran )(0,0);
            //speedup variance
            double res = 0;
            for (size_t k1 = 0; (long) k1 < vec_ran.rows(); k1++) {
                if (vec_ran(k1, 0) == 0) continue;
                for (size_t k2 = 0; (long) k2 < vec_ran.rows(); k2++) {
                    if (vec_ran(k2, 0) == 0) continue;
                    res += wran2 * cov(k1, k2);
                }
            }
            //LOG_DEBUG("res compare "<< res<<"\t"<<res2);
            //cin.get();
            //LOG_DEBUG("cicle  "<<i<<" var "<< res);
            if (i == 0) {
                current_res = res;
                cicle_idx = i;
                best_random_opt = vrandom;
                //LOG_DEBUG("new best at "<<i<<" cicle - var "<< res);
            } else {
                if (res < current_res) {
                    current_res = res;
                    cicle_idx = i;
                    best_random_opt = vrandom;
                    //LOG_DEBUG("new best at "<<i<<" cicle - var "<< res);
                }
            }

        }
        LOG_DEBUG("best at cicle " << cicle_idx << " var " << current_res);
        timeseries bestopt;
        for (auto &i1: best_random_opt) {
            LOG_DEBUG("best : "<<aller.getName(i1));
            if (bestopt.getSeriesLen()==0) bestopt=aller.getserie(i1);
            else bestopt=bestopt.merge(aller.getserie(i1));
        }
        return bestopt;
    }


    WalkForwardMinimumVariance(const vector<security>& securities,string params_file="" ):aller(init(securities,params_file))
    {

    } //<=0 means all


    void run(size_t train_window,size_t test_window )
    {
        Eigen::MatrixXd cov;
        std::stringstream  ofstatistics;
        ofstatistics<<"\n*** Trading Sim WalkForward minimum variance ***";
        ofstatistics<<"\nparameters:";
        ofstatistics<<"\nmin_samples "<<min_samples;
        ofstatistics<<"\nmax_seconds_gap "<<max_seconds_gap;
        ofstatistics<<"\nmax_seconds_before_now "<<max_seconds_before_now;
        ofstatistics<<"\nmin_mean_liquidity "<<min_mean_liquidity;
        ofstatistics<<"\nmin_train_serie "<<min_train_serie;
        ofstatistics<<"\nfixed_test_serie_random_opt "<<fixed_test_serie_random_opt;
        ofstatistics<<"\nsharpe_treshold "<<sharpe_treshold;
        ofstatistics<<"\ntransaction_penalty_pc "<<transaction_penalty_pc;
        ofstatistics<<"\nmax_tests_variance "<<max_tests_variance;
        ofstatistics<<"\ntrain_window "<<train_window;
        ofstatistics<<"\ntest_window "<<test_window;
        ofstatistics<<"\ncycles_random_opt "<<cycles_random_opt;
        if (aller.getSeriesLen()<min_train_serie) THROW_EXCEPTION("too  few series :"<<aller.getSeriesLen());
        if (aller.getLength()< (train_window+test_window)) THROW_EXCEPTION("too  few data: "<<aller.getLength());
        LOG_DEBUG("all serie "<<aller.getSeriesLen());
        LOG_DEBUG("all (er) length "<<aller.getLength());

        size_t pos=0;
        std::map<time_t,double> test_equity;
        double outofmarket=0;

        //MAIN LOOP
        while (true)
        {
            LOG_DEBUG("\n\n***training session begin*** tid " <<std::this_thread::get_id());
            timeseries trs=aller.sub(pos,train_window);
            timeseries trsbest;
            map<size_t,size_t> mapidx;
            for (size_t k=0; k<trs.getSeriesLen(); k++)
            {
                if (trs.sharpe(train_window-1).getLastValue(k)>=sharpe_treshold)
                {
                    if (trsbest.getSeriesLen()==0)
                        trsbest=trs.getserie(k);
                    else trsbest=trsbest.merge(trs.getserie(k));
                    mapidx[trsbest.getSeriesLen()-1]=k;
                }
            }
            LOG_DEBUG("training security no "<<trsbest.getSeriesLen());
            LOG_DEBUG("training security len "<<trsbest.getLength());
            if ( (trsbest.getSeriesLen()>=min_train_serie) && (fixed_test_serie_random_opt<=trsbest.getSeriesLen() ) )  //THROW_EXCEPTION("too few series "<<trsbest.getSeriesLen());
            {

                cov=trsbest.cov();
                timeseries tests,teststemp;
                double ws=0;

                //random engine
                std::default_random_engine generator(time(0));
                std::vector<size_t> vrandom;
                std::uniform_int_distribution<int> distribution(0,trsbest.getSeriesLen()-1);
                auto dice = std::bind ( distribution, generator );
                vector<size_t> best_random_opt;

                //if (fixed_test_serie_random_opt>=trsbest.getSeriesLen()) THROW_EXCEPTION("too few series in brute force random generator");
                Eigen::MatrixXd vec_ran=Eigen::MatrixXd::Zero((cov).rows(),1);
                double current_res=0;
                size_t cicle_idx=0;
                for (size_t i=0; i<cycles_random_opt; i++)
                {
                    vrandom.clear();
                    while (vrandom.size()<fixed_test_serie_random_opt)
                    {
                        size_t t=dice();
                        if (vrandom.empty())
                        {
                            vrandom.push_back(t);
                            continue;
                        }
                        if (std::find(vrandom.begin(), vrandom.end(), t) != vrandom.end()) continue;
                        vrandom.push_back(t);
                    }
                    vec_ran.fill(0);
                    double wran=1.0/fixed_test_serie_random_opt;
                    double wran2=wran*wran;
                    for (size_t j=0; j<fixed_test_serie_random_opt; j++) vec_ran(vrandom.at(j),0)=wran;

                    //double res=(vec_ran.transpose()*(*cov)*vec_ran )(0,0);
                    //speedup variance
                    double res=0;
                    for (size_t k1=0; (long)k1<vec_ran.rows(); k1++)
                    {
                        if (vec_ran(k1,0)==0) continue;
                        for (size_t k2=0; (long)k2<vec_ran.rows(); k2++)
                        {
                            if (vec_ran(k2,0)==0) continue;
                            res+=wran2* cov(k1,k2);
                        }
                    }
                    //LOG_DEBUG("res compare "<< res<<"\t"<<res2);
                    //cin.get();
                    //LOG_DEBUG("cicle  "<<i<<" var "<< res);
                    if (i==0)
                    {
                        current_res=res;
                        cicle_idx=i;
                        best_random_opt=vrandom;
                        //LOG_DEBUG("new best at "<<i<<" cicle - var "<< res);
                    }
                    else
                    {
                        if (res<current_res)
                        {
                            current_res=res;
                            cicle_idx=i;
                            best_random_opt=vrandom;
                            //LOG_DEBUG("new best at "<<i<<" cicle - var "<< res);
                        }
                    }

                }
                LOG_DEBUG("best at cicle "<<cicle_idx<<" var "<<current_res);
                ws=fixed_test_serie_random_opt;
                for (size_t k=0 ; k< best_random_opt.size(); k++)
                {
                    if (tests.getSeriesLen()<1) tests=aller.getserie(mapidx[best_random_opt.at(k)]);
                    else tests=tests.merge(aller.getserie(mapidx[best_random_opt.at(k)]));
                }

                teststemp=tests.sub(pos,train_window);
                LOG_DEBUG("teststemp "<<teststemp.getSeriesLen() <<"\t"<<teststemp.getLength());
                Eigen::MatrixXd vec=Eigen::MatrixXd::Zero(teststemp.getSeriesLen(),1);
                for (size_t k=0; k<teststemp.getSeriesLen(); k++) vec(k,0)=1.0/teststemp.getSeriesLen();
                double test_variance=(vec.transpose()* (teststemp.cov()) *vec)(0,0) ;
                LOG_DEBUG("size "<<ws<<"\t all "<<trsbest.getSeriesLen() <<"\t test variance "<<test_variance );
               //for (int k=0; k<bi.length(); k++) if (bi.gene(k)>0) vec(k,0)=1.0/ws;

                if (test_variance<=max_tests_variance)
                {

                    tests=tests.sub(pos+train_window,test_window);
                    LOG_DEBUG("test security no "<<tests.getSeriesLen());
                    LOG_DEBUG("test security len "<<tests.getLength());
                    if (test_equity.size()==0) test_equity[trsbest.getLastDate()]=1.0; //set initial equity
                    double final_test_equity=test_equity.rbegin()->second;
                    for (size_t i=0; i<tests.getLength(); i++)
                    {
                        double ptot=0;
                        for (size_t j=0; j<tests.getSeriesLen(); j++)
                        {
                            ptot+=tests(i,j)/100.0;
                        }
                        ptot/=ws;
                        double previous=test_equity.rbegin()->second;
                        test_equity[tests.getDateTime(i)]=previous*(1+ptot);


                    }
                    final_test_equity=test_equity.rbegin()->second/final_test_equity;
                    LOG_DEBUG("train period "<< timeseries::date2string(trsbest.getFirstDate())<<" to "<<timeseries::date2string(trsbest.getLastDate()));
                    LOG_DEBUG("test period "<< timeseries::date2string(tests.getFirstDate())<<" to "<<timeseries::date2string(tests.getLastDate()));
                    LOG_DEBUG("train samples "<<trsbest.getLength());
                    LOG_DEBUG("test samples "<<tests.getLength());
                    LOG_DEBUG("test equity "<<final_test_equity);
                    test_equity[test_equity.rbegin()->first]=test_equity.rbegin()->second*(1-transaction_penalty_pc);
                    LOG_DEBUG("cumulative equity = "<< test_equity.rbegin()->second);
                    LOG_DEBUG("***training session end***");
                }
                else
                {
                    timeseries ts1=aller.sub(pos+train_window,test_window);
                    LOG_DEBUG("test period "<< timeseries::date2string(ts1.getFirstDate())<<" to "<<timeseries::date2string(ts1.getLastDate()));
                    LOG_DEBUG("train samples "<<trs.getLength());
                    LOG_DEBUG("test samples "<<ts1.getLength());
                    LOG_DEBUG("test equity = 1 -- FLAT due to max variance --");
                    outofmarket+=difftime(ts1.getLastDate(),ts1.getFirstDate());

                    //cumequity*=(1.0-transaction_penalty_pc);
                    double previous=test_equity.rbegin()->second;
                    for (size_t k=0; k<ts1.getLength(); k++) test_equity[ts1.getDateTime(k)]=previous;
                    LOG_DEBUG("cumulative equity = "<< test_equity.rbegin()->second);
                    LOG_DEBUG("\n***training session end***");



                }
            }
            else   //FLAT
            {
                LOG_DEBUG("train period "<< timeseries::date2string(trs.getFirstDate())<<" to "<<timeseries::date2string(trs.getLastDate()));
                timeseries ts1=aller.sub(pos+train_window,test_window);
                LOG_DEBUG("test period "<< timeseries::date2string(ts1.getFirstDate())<<" to "<<timeseries::date2string(ts1.getLastDate()));
                LOG_DEBUG("train samples "<<trs.getLength());
                LOG_DEBUG("test samples "<<ts1.getLength());
                string sdbg="test equity = 1 -- FLAT due to ";
                if (trsbest.getSeriesLen()<min_train_serie) sdbg +=" trsbest.getSeriesLen()< min_train_serie :"+std::to_string(trsbest.getSeriesLen())+"<"+std::to_string(min_train_serie);
                else sdbg+=" fixed_test_serie_random_opt>trsbest.getSeriesLen() : "+ std::to_string(fixed_test_serie_random_opt)+">"+std::to_string(trsbest.getSeriesLen());
                sdbg+=" --";
                LOG_DEBUG(sdbg);

                outofmarket+=difftime(ts1.getLastDate(),ts1.getFirstDate());
                //cumequity*=(1.0-transaction_penalty_pc);
                double previous=test_equity.rbegin()->second;
                for (size_t k=0; k<ts1.getLength(); k++) test_equity[ts1.getDateTime(k)]=previous;
                LOG_DEBUG("cumulative equity = "<< test_equity.rbegin()->second);
                LOG_DEBUG("\n***training session end***");
            }

            ofstatistics<<"\ntrain period "<< timeseries::date2string(trs.getFirstDate())<<" to "<<timeseries::date2string(trs.getLastDate());
            timeseries ts1b=aller.sub(pos+train_window,test_window);
            ofstatistics<<"test period "<< timeseries::date2string(ts1b.getFirstDate())<<" to "<<timeseries::date2string(ts1b.getLastDate());
            ofstatistics<<"\ntrain samples "<<trs.getLength()<<"\ttest samples "<<ts1b.getLength();
            ofstatistics<<"\ncumulative equity "<<test_equity.rbegin()->second;

            pos+=test_window;
            if  (( pos+test_window+train_window) >= aller.getLength()) break;

        }

        Eigen::MatrixXd test_equity_matrix(test_equity.size(),1);
        std::vector<string> test_equity_serie_name;
        std::vector<time_t> test_equity_datetime;
        size_t increment1=0;
        for (std::map<time_t,double>::iterator k=test_equity.begin(); k!=test_equity.end(); k++)
        {
            test_equity_datetime.push_back(k->first);
            test_equity_matrix(increment1,0)=k->second;
            increment1++;
        }
        test_equity_serie_name.push_back("test_equity");

        timeseries equity(test_equity_serie_name,test_equity_datetime,timeseries::freq_enum::daily,test_equity_matrix);
        //equity.toCSV(s_of_statistics+"_equity.csv");
        double maxm=0,maxdd=0;
        for (size_t k1=0; k1<equity.getLength(); k1++)
        {
            if (k1==0) maxm=equity(0,0);
            else
            {
                if (equity(k1,0)>maxm) maxm=equity(k1,0);
                double dd=equity(k1,0)<maxm? (maxm-equity(k1,0) )/maxm:0;
                if (maxdd<dd) maxdd=dd;
            }
        }

        ofstatistics<<"\ntest from "<<timeseries::date2string(equity.getFirstDate())<<" to "<<timeseries::date2string(equity.getLastDate());
        ofstatistics<<"\ntest "<<difftime(equity.getLastDate(),equity.getFirstDate())/60/60/24<<" days";
        ofstatistics<<"\nout of market "<<outofmarket/60/60/24 <<" days";
        ofstatistics<<"\nmaxdd %"<<maxdd*100.0;
        ofstatistics<<"\nfinal equity"<<equity.getLastValue(0);
        ofstatistics<<"\n*** CORRECTLY CLOSED ***";
        retstats.final_equity=equity.getLastValue(0);
        retstats.inmarket_pc= 100*(difftime(equity.getLastDate(),equity.getFirstDate())/60/60/24 - outofmarket/60/60/24 )/(difftime(equity.getLastDate(),equity.getFirstDate())/60/60/24);
        retstats.maxdd_pc=maxdd*100;
        retstats.test_window=test_window;
        retstats.train_window=train_window;
        retstats.equity=equity;
        retstats.log=ofstatistics.str();

    }

};

#endif // WALKFORWARDMV_HPP_INCLUDED
