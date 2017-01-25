#ifndef TIMESERIES_HPP_INCLUDED
#define TIMESERIES_HPP_INCLUDED
#include <vector>
#include <string>
#include <ctime>
#include "logger.hpp"
#include "Eigen/Core"
#include "Eigen/Dense"



//using namespace std;


class timeseries
{
public: enum freq_enum {daily,weekly,monthly};
private:
    std::vector<time_t> dates;
//    enum freq_enum {daily,weekly,monthly};
    freq_enum  freq;
    Eigen::MatrixXd M;
    std::vector<std::string> names;

public:
    static std::string date2string(time_t date) ;
    timeseries();//permette 0-length serie!!!
    timeseries(const std::vector<std::string> &names,const std::vector<time_t> &dates,const freq_enum & f,const Eigen::MatrixXd M);
    const timeseries getserie(unsigned i) const;
    const timeseries head( long i) const;
    const timeseries sub( size_t from, size_t nrows) const;
    const double operator()(size_t i,size_t j) const;
    const std::vector<std::string> getNames() const;
    const std::string getName(unsigned i ) const;
    const size_t getLength() const;
    const size_t getSeriesLen() const;
    const std::vector<time_t> getDateTime() const;
    const time_t getDateTime(unsigned i) const;
    const time_t getFirstDate() const;
    const time_t getLastDate() const;
    const double getFirstValue(size_t serie) const;
    const double getLastValue(size_t serie) const;
    const double getDateGapInSeconds() const ;
    const freq_enum getFreq() const;
    const Eigen::MatrixXd getMatrix() const;
    const timeseries normalize() const;//ritorna le serie normalizzate (x - mean(x))/std(x))
    const timeseries diff() const;//v(t)-v(t-1)
    const timeseries diff(const timeseries &T) const;
    const timeseries prod(const timeseries &T) const;
    const timeseries quot(const timeseries &T) const;
    const timeseries daterange(size_t from, size_t to) const ;
    const timeseries daterange(time_t from, time_t to) const;
    const timeseries merge(const timeseries &T) const;
    const timeseries lag(long l) const;
    const timeseries mlagdelay(unsigned steps) const;
    const timeseries er(unsigned lag=1,double multiplier=100,bool loger=false) const;//excess return with lag period difference 100*log(v(t)/v(t-1)) or 100*(v(t)-v(t-1))/v(t-1)
    const timeseries rowkron() const;
    const Eigen::MatrixXd mean() const;
    const Eigen::MatrixXd cov() const;
    const Eigen::MatrixXd corr() const;
    void plot(size_t serie=0) const;
    Eigen::MatrixXd max() const ;//massimo di ogni colonna
    Eigen::MatrixXd min() const ;//minimo di ogni colonna
    Eigen::MatrixXd maxabs() const ;//massimo di ogni colonna in valore assoluto
    Eigen::MatrixXd minabs() const ;//minimo di ogni colonna in valore assoluto
    const timeseries sma(size_t period) const;//simple moving average
    const timeseries ema(double alpha=0.7) const ;//exponential moving average
    const timeseries wma(size_t period) const;//weighted moving average
    const timeseries mvariance(size_t period) const ;//mobile variance
    const timeseries sharpe(size_t period) const;//mobile mean/std
    friend std::ostream& operator << (std::ostream& os,const timeseries&  ts);
    void toCSV(std::string filename,std::string delimiter=";") const;
    const bool one_tail_t_test_corr(size_t i,size_t j, double confidence=0.05) const ;//test significant correlation between series
    const bool dateexists(time_t d) const ;
    const size_t dateidx(time_t d) const;
    const double pearsoncorr2(size_t xcol,size_t ycol) const;
    const bool timeseriefilter(size_t min_len,double maxtimegapinsec, double maxvaluegappct, double maxtimebefornowinsec) const;
};


#endif // TIMESERIES_HPP_INCLUDED
