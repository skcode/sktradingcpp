#ifndef OPT_HPP_INCLUDED
#define OPT_HPP_INCLUDED

#include <cstdio>
#include <iostream>
#include <fstream>
#include <ga/ga.h>
#include "security.h"
#include "logger.hpp"
#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
namespace MinimumVariance {

Eigen::MatrixXd cov;

int quantizationbits=5;
int popsize  = 10000;
int ngen     = 400;
float pmut   = 0.01;
float pcross = 0.6;



float mvobjective(GAGenome & c)
{
  GABin2DecGenome & genome = (GABin2DecGenome &)c;

  Eigen::MatrixXd v(cov.rows(),1);
  for (int k=0;k<cov.rows();k++) v(k,0)=genome.phenotype(k);
  double mc=v.sum();
  //if (mc>maxstock) return 0;
  for (int k=0;k<cov.rows();k++) v(k,0)/=mc;
  //LOG_DEBUG("v\n"<<v);
  //LOG_DEBUG("cov\n"<<cov);
  Eigen::MatrixXd res=v.transpose()*cov*v;
  //LOG_DEBUG("res\n"<<res)
  if (res(0,0)<=0) return 0; else
  return 1.0/res(0,0);
}
float mvobjectivebin(GAGenome & c)
{
  GA1DBinaryStringGenome & genome = (GA1DBinaryStringGenome &)c;



  Eigen::MatrixXd v=Eigen::MatrixXd::Zero(cov.rows(),1);
  double mc=0;
  for (int k=0;k<genome.length();k++) mc+= genome.gene(k);
  for (int k=0;k<genome.length();k++) if (genome.gene(k)>0) v(k,0)=1.0/ mc;
  double length_penalty=1;//(mc*mc);//=mc;//*mc;mc/cov.rows();
  Eigen::MatrixXd res=v.transpose()*cov*v;
  if (res(0,0)<=0) return 0; else
  return (1.0/ (res(0,0)*length_penalty ));
}


void MinimumVariance(const vector<security> &securities,size_t window=200,size_t maxdaysgap=7,bool neartodate=true,bool bindec=true){

    timeseries all;
    time_t t = time(0);   // get time now
    double meansharpe=0;
    unsigned meansharpesample=0;
    for (const security &s:securities) {
        LOG_DEBUG("symbol "<<s.getName());
        LOG_DEBUG("length "<<s.ts.getLength());
        if (s.ts.getLength()< (window*2)) continue;
        LOG_DEBUG("dategap "<<s.ts.head(window*2).getDateGapInSeconds()/60/60/24);
        if ((s.ts.head(window*2).getDateGapInSeconds()/60/60/24)>maxdaysgap ) continue;
        LOG_DEBUG("lastdate "<<timeseries::date2string(s.ts.getLastDate()))
        if (s.ts.getLastDate()<(t-7*24*60*60 ) ) continue; //at least 7 days from now
        double volmean=0;
        for (size_t k=s.ts.getLength()-11;k<s.ts.getLength();k++) volmean+=s.close(k)*s.volume(k);
        volmean/=10;
        if (volmean<10000) continue;
        timeseries tsadd;
        try {
            tsadd=s.ts.getserie(3).er();
        }catch (...) {
            continue;
        }
        //tsadd.sharpe(200).toCSV("/tmp/test.csv");
        double msharpe200=tsadd.sharpe(200).getLastValue(0);
        meansharpe+=msharpe200;
        meansharpesample++;
        LOG_DEBUG("sharpe "<<msharpe200);
        if (msharpe200<0.01) continue;
        if (all.getSeriesLen()==0) all=tsadd;//close
        else all=all.merge(tsadd);
    }
    meansharpe/=meansharpesample;
    if (all.getLength()<window) THROW_EXCEPTION("too  few data:"<<all.getLength());
    LOG_DEBUG("all serie "<<all.getSeriesLen());
    LOG_DEBUG("all length "<<all.getLength());
    LOG_DEBUG("mean sharpe "<<meansharpe);
    all=all.head(window);
    cov= all.cov();
    if (!bindec){
    GABin2DecPhenotype map;
    for (size_t k=0;k<all.getSeriesLen();k++) map.add(quantizationbits, 0, 1);
    GABin2DecGenome genome(map, mvobjective);
    GASimpleGA ga(genome);
    ga.populationSize(popsize);
    ga.nGenerations(ngen);
    ga.pMutation(pmut);
    ga.pCrossover(pcross);
    ga.evolve();
  cout << "\nmax fitness:\n" << 1.0/ga.statistics().maxEver() << "\n";
  cout << "\nmin fitness:\n" << 1.0/ga.statistics().minEver() << "\n";
  GABin2DecGenome bi=(GABin2DecGenome &)ga.statistics().bestIndividual() ;
  std::vector<double> w;
double ws=0;
  for (size_t k=0 ;k< all.getSeriesLen();k++)  {w.push_back(bi.phenotype(k));ws+=bi.phenotype(k);}
  for (size_t k=0 ;k< all.getSeriesLen();k++) {w[k]=w.at(k)/ws; cout <<"\nweight "<<k<<"\t"<<all.getName(k)<< "\t"<<w.at(k);}
  cout.flush();
  }else  {
    GA1DBinaryStringGenome genomebin(cov.rows(), mvobjectivebin);
    GASimpleGA ga(genomebin);
    ga.populationSize(popsize);
    ga.nGenerations(ngen);
    ga.pMutation(pmut);
    ga.pCrossover(pcross);
    ga.evolve();
  cout << "\nmax fitness:\n" << 1.0/ga.statistics().maxEver() << "\n";
  cout << "\nmin fitness:\n" << 1.0/ga.statistics().minEver() << "\n";
  GA1DBinaryStringGenome bi=(GA1DBinaryStringGenome  &)ga.statistics().bestIndividual() ;

double ws=0;
  for (int k=0 ;k< bi.length();k++)  {ws+=bi.gene(k);}
    Eigen::MatrixXd v=Eigen::MatrixXd::Zero(cov.rows(),1);
    for (int k=0;k<bi.length();k++) if (bi.gene(k)>0) v(k,0)=1.0/ws;
  cout <<"\nsize "<<ws<<"\t all "<<all.getSeriesLen() <<"\t variance "<<(v.transpose()*cov*v)(0,0) ;
  timeseries tres;
  for (int k=0 ;k< bi.length();k++) { if (bi.gene(k)>0) {cout <<"\nweight "<<1.0/ws<<"\t"<<all.getName(k);if (tres.getSeriesLen()==0) tres=all.getserie(k);else tres=tres.merge(all.getserie(k)); }}
  tres.toCSV("/tmp/tres.csv");
  cout.flush();

  }
}


}//end namespace
#endif // OPT_HPP_INCLUDED
