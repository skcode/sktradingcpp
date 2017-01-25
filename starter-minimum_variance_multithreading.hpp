#ifndef STARTER_MINIMUM_VARIANCE_MULTITHREADING_HPP_INCLUDED
#define STARTER_MINIMUM_VARIANCE_MULTITHREADING_HPP_INCLUDED

#include <thread>
#include <chrono>
#include <mutex>
#include "walkforwardmv.hpp"
using namespace std;

namespace minvariance_mt {
    static std::mutex tfmtx;
    static void threadfunc(ofstream & of,ofstream &logf,WalkForwardMinimumVariance &wfmv,size_t i,size_t j)
    {
        try
        {
            wfmv.run(i,j);
            minvariance_mt::tfmtx.lock();
            of<<"\n"<< wfmv.retstats.train_window<<";"<<wfmv.retstats.test_window<<";"<< wfmv.retstats.final_equity<<";"<<wfmv.retstats.maxdd_pc<<";"<<wfmv.retstats.inmarket_pc<<";"<<timeseries::date2string(wfmv.retstats.equity.getFirstDate()) <<";"<<timeseries::date2string(wfmv.retstats.equity.getLastDate())<<";" ;
            of <<wfmv.get_cycles_random_opt()<<";"<<wfmv.get_fixed_test_serie_random_opt()<<";";
            of << wfmv.get_max_seconds_before_now()<<";"<<wfmv.get_max_seconds_gap()<<";"<<wfmv.get_max_tests_variance()<<";"<<wfmv.get_min_mean_liquidity()<<";";
            of<<wfmv.get_min_samples()<<";"<<wfmv.get_min_train_serie()<<";"<<wfmv.get_sharpe_treshold()<<";"<<wfmv.get_transaction_penalty_pc();
            of.flush();
            logf<<"\n\n***************************************************************************\n";
            logf<<wfmv.retstats.log;
            logf<<"\n***************************************************************************\n";
            logf.flush();
            minvariance_mt::tfmtx.unlock();
        }
        catch (std::exception e)
        {
            LOG_WARN("exception :"<<e.what());
        }

    }

    static void testmv(string db,string market,string params="",size_t trainfrom=60)
    {

        if (!(market=="MLSE" or market=="NYSE")) THROW_EXCEPTION("market not available : "<<market);
        unsigned int nthreads = std::thread::hardware_concurrency();
        vector<thread> vt;
        string basepath="/mnt/sdb1/skdata/dev/tests/";
        string ofstr=basepath+timeseries::date2string(time(0))+"_"+market+"_results.csv";
        string logfstr=basepath+timeseries::date2string(time(0))+"_"+market+"_results_log.txt";
        LOG_DEBUG("output files "<<ofstr<<"\t"<<logfstr);
        std::ofstream of(ofstr);
        std::ofstream logf(logfstr);
        if (!of.good() || !logf.good()) THROW_EXCEPTION("bad output files "<<ofstr<<"\t"<<logfstr);
        of<<"\n"<<market<<" wf test";
        of<<"\ntrain_window;test_window;final_equity;maxdd;inmarket;test_start_date;test_end_date;cycles_random_opt;fixed_test_serie_random_opt;";
        of<<"max_seconds_before_now;max_seconds_gap;max_tests_variance;min_mean_liquidity;min_samples;min_train_serie;sharpe_treshold;transaction_penalty_pc";
        of.flush();
        vector<security> s = market=="MLSE"? WalkForwardMinimumVariance::getSecurities(db,"","EUR","MLSE","STOCK"):WalkForwardMinimumVariance::getSecurities(db,"","USD","NYSE","STOCK");
        WalkForwardMinimumVariance wfmv(s,params);

        try
        {
            while (true)
                for (size_t i=trainfrom; i<=300; i=i+5) //train window
                {
                    for (size_t j=20; j<=120 ; j=j+5) //test window
                        if (j>i) continue;
                        else
                        {
                            if (vt.size()>=nthreads)
                            {
                                for (size_t k=0; k<vt.size(); k++) if (vt.at(k).joinable()) vt.at(k).join();
                                vt.clear();
                                of.flush();
                                logf.flush();
                                //std::this_thread::sleep_for(std::chrono::milliseconds(30000));//sleep for a while
                            }
                            vt.push_back( std::thread( threadfunc,std::ref(of),std::ref(logf),std::ref(wfmv),i,j ) );

                        }

                }
        }
        catch (std::exception e)
        {
            LOG_WARN("exception :"<<e.what());
        }
        for (size_t k=0; k<vt.size(); k++) if (vt.at(k).joinable()) vt.at(k).join();
        of.close();
        logf.close();
    }

}

#endif // STARTER-MINIMUM_VARIANCE_MULTITHREADING_HPP_INCLUDED
