#include "timeseries.hpp"
#include <vector>
#include <string>
#include <ctime>
#include <ostream>
#include <sstream>
#include <iostream>
#include <fstream>
#include "logger.hpp"
#include "Eigen/Core"
#include "Eigen/Dense"
#include <plplot/plplot.h>
#include <plplot/plstream.h>
using namespace std;

timeseries::timeseries()
{
    dates=vector<time_t>(0);
    freq=freq_enum::daily;//default
    names=vector<string>(0);
}

timeseries::timeseries(const vector<string> &names,const vector<time_t> &dates,const freq_enum &freq,const Eigen::MatrixXd M)
{
    if ((long)dates.size()!=M.rows() || (long)names.size()!=M.cols() )
        THROW_EXCEPTION("bad arguments : "<<dates.size()<<"\t"<<names.size()<<"\t"<<M.rows()<<"\t"<<M.cols());
    for (std::vector<time_t>::size_type  i =1; i<dates.size(); i++)
    {
        if (dates.at(i)<=dates.at(i-1))
            THROW_EXCEPTION("unsorted dates "<<dates.at(i)<<" - "<<dates.at(i-1));
    }
    this->dates=dates;
    this->names=names;
    this->freq=freq;
    this->M=M;
}

const timeseries timeseries::getserie(unsigned i) const
{
    if (i>=this->names.size()) THROW_EXCEPTION("bad index "<<i);
    Eigen::MatrixXd newM=M.col(i);
    vector<string> newnames;
    newnames.push_back(names.at(i));
    return timeseries(newnames,dates,freq,newM);

}
const timeseries timeseries::head( long i) const
{
    if (i>M.rows() || i<1) THROW_EXCEPTION("bad index "<<i);
    Eigen::MatrixXd newM=M.bottomRows(i);
    vector<time_t> newdates;
    for (std::vector<time_t>::size_type k=dates.size()-(std::vector<time_t>::size_type) i; k<dates.size(); k++)
        newdates.push_back(dates.at(k));
    return timeseries(names,newdates,freq,newM);

}

const timeseries timeseries::sub( size_t from, size_t nrows) const
{
    size_t to=from +nrows-1;
    if (from<0 || from >=to || (long)to >=M.rows() ) THROW_EXCEPTION("bad index "<<from<<"\t"<<to);
    Eigen::MatrixXd newM=M.middleRows(from,nrows);
    vector<time_t> newdates;
    for (size_t k=from; k<=to; k++) newdates.push_back(dates.at(k));
    //for (std::vector<time_t>::size_type k=dates.size()-(std::vector<time_t>::size_type) i; k<dates.size(); k++)
    //  newdates.push_back(dates.at(k));
    return timeseries(names,newdates,freq,newM);

}





const double timeseries::operator()(size_t i,size_t j) const
{
    if (i>=(size_t)M.rows() || j>=(size_t)M.cols())throw std::invalid_argument("bad index");
    return M(i,j);
}


const vector<string> timeseries::getNames() const
{
    return this->names;
}
const string timeseries::getName(unsigned i ) const
{
    if (i>=this->names.size()) throw std::invalid_argument("bad index");
    return names[i];
}
const size_t timeseries::getLength() const
{
    return this->dates.size();
}
const size_t timeseries::getSeriesLen() const
{
    return this->names.size();
}


const double timeseries::getDateGapInSeconds() const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    if (dates.size()<=1) return 0;
    double m=0;
    for (size_t i=1; i<dates.size(); i++)
    {
        double t=difftime(dates.at(i),dates.at(i-1));
        if (t>m)
            m=t;
    }
    return m;
}


const vector<time_t> timeseries::getDateTime() const
{
    return this->dates;
}
const time_t timeseries::getDateTime(unsigned i) const
{
    if (i>=this->dates.size()) throw std::invalid_argument("bad index");
    return this->dates[i];
}
const time_t timeseries::getFirstDate() const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    return this->dates[0];
}
const time_t timeseries::getLastDate() const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    return this->dates[this->dates.size()-1];
}

const double timeseries::getFirstValue(size_t serie) const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    if (this->getSeriesLen()<=serie) THROW_EXCEPTION("index out of bound "<<serie);

    return this->M(0,serie);
}
const double timeseries::getLastValue(size_t serie) const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    if (this->getSeriesLen()<=serie) THROW_EXCEPTION("index out of bound "<<serie);

    return this->M(this->getLength()-1,serie);
}



const timeseries::freq_enum timeseries::getFreq() const
{
    return this->freq;
}

const Eigen::MatrixXd timeseries::getMatrix() const
{
    return this->M;
}

const timeseries timeseries::normalize() const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    Eigen::MatrixXd mean= this->mean();
    Eigen::MatrixXd cov= this->cov();
    Eigen::MatrixXd newM=M;
    for (size_t i=0;i<M.rows();i++)
        for (size_t j=0;j<M.cols();j++) {
            newM(i,j)=(M(i,j)-mean(0,j))/sqrt(cov(j,j));
        }
    vector<string> newnames(names.size());
    for (size_t i=0; i<names.size(); i++) newnames[i]="normalize("+names[i]+")";    
    return timeseries(newnames,dates,freq,newM);
}


const timeseries timeseries::diff() const
{
    if (this->getLength()<2) THROW_EXCEPTION("too short timeserie");
    Eigen::MatrixXd newM=Eigen::MatrixXd(M.rows()-1,M.cols());
    for (size_t i=1;i<M.rows();i++)
        for (size_t j=0;j<M.cols();j++) {
            newM(i-1,j)=(M(i,j)-M(i-1,j));
        }
    vector<string> newnames(names.size());
    vector<time_t> newdates(this->dates.size()-1);
    for (std::vector<time_t>::size_type i=1; i<dates.size(); i++) newdates[i-1]=dates.at(i);    
    for (std::vector<time_t>::size_type i=0; i<names.size(); i++) newnames[i]="diff("+names[i]+")";    
    return timeseries(newnames,newdates,freq,newM);
}


const timeseries timeseries::diff(const timeseries &T) const
{
    if (this->freq!=T.freq) THROW_EXCEPTION("no matching freq " <<this->freq<<" - "<<T.freq);
    if (M.cols()!= T.M.cols() || M.rows()!=T.M.rows()) THROW_EXCEPTION("no matching size :"<<M.rows()<<"x"<<M.cols()<<" vs " <<T.M.rows()<<"x"<<T.M.cols());
    for (std::vector<time_t>::size_type i=0; i<dates.size(); i++)
        if (dates[i]!=T.dates[i]) THROW_EXCEPTION("no matching dates :"<<dates[i]<<" vs "<<T.dates[i]);
    vector<string> newnames(names.size());
    for (std::vector<time_t>::size_type i=0; i<names.size(); i++) newnames[i]="("+names[i]+"-"+T.names[i]+")";
    Eigen::MatrixXd newM=M-T.M;
    return timeseries(newnames,dates,freq,newM);
}




const timeseries timeseries::prod(const timeseries &T) const
{
    if (this->freq!=T.freq) THROW_EXCEPTION("no matching freq " <<this->freq<<" - "<<T.freq);
    if (M.cols()!= T.M.cols() || M.rows()!=T.M.rows()) THROW_EXCEPTION("no matching size :"<<M.rows()<<"x"<<M.cols()<<" vs " <<T.M.rows()<<"x"<<T.M.cols());


    for (std::vector<time_t>::size_type i=0; i<dates.size(); i++)
        if (dates[i]!=T.dates[i]) THROW_EXCEPTION("no matching dates :"<<dates[i]<<" vs "<<T.dates[i]);

    vector<string> newnames(names.size());
    for (std::vector<time_t>::size_type i=0; i<names.size(); i++) newnames[i]="("+names[i]+".*"+T.names[i]+")";
    Eigen::MatrixXd newM=M.cwiseProduct(T.M);
    return timeseries(newnames,dates,freq,newM);
}

const timeseries timeseries::quot(const timeseries &T) const
{
    if (this->freq!=T.freq) THROW_EXCEPTION("no matching freq " <<this->freq<<" - "<<T.freq);
    if (M.cols()!= T.M.cols() || M.rows()!=T.M.rows()) THROW_EXCEPTION("no matching size :"<<M.rows()<<"x"<<M.cols()<<" vs " <<T.M.rows()<<"x"<<T.M.cols());


    for (std::vector<time_t>::size_type i=0; i<dates.size(); i++)
        if (dates[i]!=T.dates[i]) THROW_EXCEPTION("no matching dates :"<<dates[i]<<" vs "<<T.dates[i]);

    vector<string> newnames(names.size());
    for (std::vector<time_t>::size_type i=0; i<names.size(); i++) newnames[i]="("+names[i]+"./"+T.names[i]+")";
    Eigen::MatrixXd newM=M.cwiseQuotient(T.M);
    return timeseries(newnames,dates,freq,newM);
}


const timeseries timeseries::merge(const timeseries &T) const
{
    vector<unsigned long> a,b;
    if (this->freq!=T.freq) THROW_EXCEPTION("no matching freq " <<this->freq<<" - "<<T.freq);

    for (std::vector<time_t>::size_type i=0; i<dates.size(); i++)
    {
        if (std::binary_search(T.dates.begin(),T.dates.end(),dates.at(i)))
        {
            a.push_back(i);
            unsigned long t1=std::lower_bound(T.dates.begin(),T.dates.end(),dates.at(i))-T.dates.begin();
            b.push_back(t1);
        }
    }
    if (a.size()==0)  THROW_EXCEPTION("no matching dates");
    vector<string> newnames;
    vector<time_t> newdates;
    for (std::vector<string>::size_type i=0; i<this->names.size(); i++) newnames.push_back(this->names.at(i));
    for (std::vector<string>::size_type i=0; i<T.names.size(); i++) newnames.push_back(T.names.at(i));

    for (std::vector<unsigned long >::size_type i=0; i< a.size(); i++) newdates.push_back(this->dates.at(a.at(i)));
    Eigen::MatrixXd newM(a.size(),newnames.size());
    for (std::vector<unsigned long >::size_type i=0; i< a.size(); i++)
    {
        for (long j=0; j< this->M.cols(); j++)
            newM(i,j)=this->M(a.at(i),j);
        for (long j=0; j< T.M.cols(); j++)
            newM(i,j+this->M.cols())=T.M(b.at(i),j);
    }
    return timeseries (newnames,newdates,this->freq,newM);


}
const timeseries timeseries::daterange(size_t from, size_t to) const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    if (to>=dates.size() || from > to) THROW_EXCEPTION("bad range :"<<from<<"\t"<<to);
    size_t newlen=to-from+1;
    vector<time_t> newdates;
    Eigen::MatrixXd newM=M.middleRows(from, newlen);
    for (size_t i=from; i<=to; i++) newdates.push_back(dates.at(i));
    return timeseries(names,newdates,this->freq,newM);
}

const timeseries timeseries::daterange(time_t from, time_t to) const
{
    if (this->getLength()==0) THROW_EXCEPTION("zero length timeserie");
    return daterange(this->dateidx(from),this->dateidx(to));
}


const timeseries timeseries::lag(long l) const
{
    if (this->dates.size()<=(unsigned)abs(l)) THROW_EXCEPTION("bad index :"<<l);
    vector<string> newnames;
    vector<time_t> newdates;
    Eigen::MatrixXd newM(this->M.rows()-abs(l),this->M.cols());
    if (l>=0)
        for (std::vector<time_t >::size_type i=l; i<this->dates.size(); i++) newdates.push_back(this->dates.at(i));
    else for (std::vector<time_t >::size_type i=0; i<this->dates.size()-abs(l); i++) newdates.push_back(this->dates.at(i));
    for (std::vector<string >::size_type i=0; i<this->names.size(); i++) newnames.push_back("LAG("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() << l) )->str()+")");
    if (l>=0) newM=M.topRows(newM.rows());
    else newM=M.bottomRows(newM.rows());
    return timeseries(newnames,newdates,this->freq,newM);
}

const timeseries timeseries::mlagdelay(unsigned steps) const
{
    if (this->dates.size()<=steps) THROW_EXCEPTION("bad index :"<<steps);
    vector<string> newnames;
    vector<time_t> newdates;
    for (std::vector<time_t >::size_type i=steps; i<this->dates.size(); i++) newdates.push_back(this->dates.at(i));
    Eigen::MatrixXd newM=Eigen::MatrixXd(newdates.size(),M.cols()*steps);
    for (unsigned k=0; k<steps; k++)
        for (std::vector<string >::size_type i=0; i<this->names.size(); i++)
            newnames.push_back("LAG("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() <<  (k+1) ) )->str()+")");
    for (long i=0; i<newM.rows(); i++)
        for (unsigned  j=0; j<steps; j++)
            for (long k=0; k<M.cols(); k++)
            {
                newM(i,j*M.cols()+k)=M(steps-j+i-1,k);
            }
    return timeseries(newnames,newdates,this->freq,newM);

}



const timeseries timeseries::er(unsigned l,double multiplier,bool loger) const
{
    if (this->dates.size()<(l+1)) THROW_EXCEPTION("too few rows:"<<this->dates.size());
    if (l==0) THROW_EXCEPTION("lag cannot be 0");
    vector<time_t> newdates;
    vector<string> newnames;
    Eigen::MatrixXd newM(this->M.rows()-l,this->M.cols());
    for (std::vector<time_t >::size_type i=l; i<this->dates.size(); i++) newdates.push_back(this->dates.at(i));
    for (std::vector<string >::size_type i=0; i<this->names.size(); i++) newnames.push_back("ER("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() <<  (l) ) )->str()+")");
    for (long i=l; i<this->M.rows(); i++)
        for (long j=0; j<this->M.cols(); j++)
            if (this->M(i-l,j)<0.00000001)  {THROW_EXCEPTION("divide by 0");}
            else newM(i-l,j)= loger ?  multiplier*log(this->M(i,j)/this->M(i-l,j)) : multiplier*(this->M(i,j)-this->M(i-l,j))/this->M(i-l,j);//
    return timeseries(newnames,newdates,this->freq,newM);
}

const timeseries timeseries::sma(size_t period) const
{
    if (dates.size()<period) THROW_EXCEPTION("period is too long :"<<period);
    if (period==0) THROW_EXCEPTION("period cannot be 0");
    vector<time_t> newdates;
    vector<string> newnames;
    Eigen::MatrixXd newM(this->M.rows()-period+1,this->M.cols());
    for (std::vector<time_t >::size_type i=period-1; i<this->dates.size(); i++) newdates.push_back(this->dates.at(i));
    for (std::vector<string >::size_type i=0; i<this->names.size(); i++) newnames.push_back("sma("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() <<  (period) ) )->str()+")");
    for (long j=0; j<newM.cols(); j++)
    {
        for (long i=0; i<newM.rows(); i++)
        {
            double m=0;
            for (long k=i ; k< (i +(long) period  ); k++ ) m+=M(k,j);
            m/=(double)period;
            newM(i,j)=m;
        }

    }
    return timeseries(newnames,newdates,this->freq,newM);
}

const timeseries timeseries::wma(size_t period) const
{
    if (dates.size()<period) THROW_EXCEPTION("period is too long :"<<period);
    if (period==0) THROW_EXCEPTION("period cannot be 0");
    vector<time_t> newdates;
    vector<string> newnames;
    double den=period*(period+1)/2.0;
    Eigen::MatrixXd newM(this->M.rows()-period+1,this->M.cols());
    for (std::vector<time_t >::size_type i=period-1; i<this->dates.size(); i++) newdates.push_back(this->dates.at(i));
    for (std::vector<string >::size_type i=0; i<this->names.size(); i++) newnames.push_back("wma("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() <<  (period) ) )->str()+")");
    for (long j=0; j<newM.cols(); j++)
    {
        for (long i=0; i<newM.rows(); i++)
        {
            double m=0;
            for (long k=i ; k< (i +(long) period  ); k++ ) m+=(k-i+1)*M(k,j);
            m/=(double)den;
            newM(i,j)=m;
        }

    }
    return timeseries(newnames,newdates,this->freq,newM);
}



const timeseries timeseries::mvariance(size_t period) const
{
    if (dates.size()<period) THROW_EXCEPTION("period is too long :"<<period);
    if (period<2) THROW_EXCEPTION("period cannot be <2");
    vector<time_t> newdates;
    vector<string> newnames;
    Eigen::MatrixXd newM(this->M.rows()-period+1,this->M.cols());
    for (std::vector<time_t >::size_type i=period-1; i<this->dates.size(); i++) newdates.push_back(this->dates.at(i));
    for (std::vector<string >::size_type i=0; i<this->names.size(); i++) newnames.push_back("mvariance("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() <<  (period) ) )->str()+")");
    for (long j=0; j<newM.cols(); j++)
    {
        for (long i=0; i<newM.rows(); i++)
        {
            double m=0,s=0;
            for (long k=i ; k< (i +(long) period  ); k++ ) m+=M(k,j);
            m/=(double)period;
            for (long k=i ; k< (i +(long) period  ); k++ ) s+= (M(k,j)-m)*(M(k,j)-m);
            s/=(double)period-1-0;
            newM(i,j)=s;
        }

    }
    return timeseries(newnames,newdates,this->freq,newM);
}


const timeseries timeseries::sharpe(size_t period) const
{
    if (dates.size()<period) THROW_EXCEPTION("period is too long :"<<period);
    if (period<2) THROW_EXCEPTION("period cannot be <2");
    vector<time_t> newdates;
    vector<string> newnames;
    Eigen::MatrixXd newM(this->M.rows()-period+1,this->M.cols());
    for (std::vector<time_t >::size_type i=period-1; i<this->dates.size(); i++) newdates.push_back(this->dates.at(i));
    for (std::vector<string >::size_type i=0; i<this->names.size(); i++) newnames.push_back("sharpe("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() <<  (period) ) )->str()+")");
    for (long j=0; j<newM.cols(); j++)
    {
        for (long i=0; i<newM.rows(); i++)
        {
            double m=0,s=0;
            for (long k=i ; k< (i +(long) period  ); k++ ) m+=M(k,j);
            m/=(double)period;
            for (long k=i ; k< (i +(long) period  ); k++ ) s+= (M(k,j)-m)*(M(k,j)-m);
            s/=(double)period-1-0;
            newM(i,j)=m/sqrt(s);
        }

    }
    return timeseries(newnames,newdates,this->freq,newM);
}



const timeseries timeseries::ema(double alpha) const
{
    if (alpha <=0 || alpha>=1) THROW_EXCEPTION("alpha must be in (0,1)");
    vector<string> newnames;
    Eigen::MatrixXd newM(this->M.rows(),this->M.cols());
    for (std::vector<string >::size_type i=0; i<this->names.size(); i++) newnames.push_back("ema("+this->names.at(i)+","+static_cast<ostringstream*>( &(ostringstream() <<  (alpha) ) )->str()+")");

    //size_t len=dates.size();
    for (long j=0; j<newM.cols(); j++)
    {
        for (long i=0; i<newM.rows(); i++)
        {
            if (i==0) newM(i,j)=M(i,j);
            else newM(i,j)=alpha*newM(i-1,j)*(1-alpha)+M(i,j);
        }

    }
    return timeseries(newnames,dates,this->freq,newM);
}




const timeseries timeseries::rowkron() const
{
    vector<string> newnames;
    Eigen::MatrixXd newM(this->M.rows(),this->M.cols()*this->M.cols());

    for (std::vector<string>::size_type i=0; i<names.size(); i++)
    {
        for (std::vector<string>::size_type k=0; k<names.size(); k++)
        {
            newnames.push_back(names.at(i)+"*"+names.at(k));
            for (long j=0; j<M.rows(); j++)
                newM(j,i*names.size()+k)=M(j,i)*M(j,k);
        }
    }

    //for (unsigned i=0;i<newnames.size();i++) cout << newnames.at(i);
    return timeseries(newnames,dates,freq,newM);
}
const Eigen::MatrixXd timeseries::mean() const
{
    return M.colwise().mean();
}


/*
	    //osservazioni per righe, variabili per colonne
    static public double[][] cov(double[][]m)throws Exception{
    	//if ( !isRectangular(m)) throw new Exception("Not rectangular matrix");
    	int n=nCols(m);
    	double[] mean=mean(m);
    	double[][] res=new double[n][n];
    	for (int j=0;j<n;j++) {
    		for (int i=j;i<n;i++){
    			double s=0;
    			for (int k=0;k<m.length;k++) s+=(m[k][j]-mean[j])*(m[k][i]-mean[i]);
    			res[i][j]=s/(m.length-1);
    		}
    	}
    	for (int j=0;j<n;j++)
    		for (int i=0;i<j;i++)
    			res[i][j]=res[j][i];
    	return res;
    }
        //osservazioni per righe, variabili per colonne
    static public double[][] corr(double[][] m)throws Exception {

    	double[][] res=cov(m);
    	double[][] newm=new double[res.length][res.length];
    	for (int i=0;i<res.length;i++)
    		for (int j=0;j<res.length;j++)
    			newm[i][j]=res[i][j]/Math.sqrt(res[i][i]*res[j][j]);
    	return newm;
    }

*/


const Eigen::MatrixXd timeseries::cov() const
{
    if (this->getLength()<1 ) THROW_EXCEPTION("length must be > 1");
    long n=M.cols();
    Eigen::MatrixXd mean=M.colwise().mean();
    Eigen::MatrixXd res(n,n);
    for (long j=0; j<n; j++)
    {
        for (long i=j; i<n; i++)
        {
            double s=0;
            for (long k=0; k<M.rows(); k++) s+=(M(k,j)-mean(j))*(M(k,i)-mean(i));
            res(i,j)=  s/(M.rows()-1);
        }
    }
    for (int j=0; j<n; j++)
        for (int i=0; i<j; i++)
            res(i,j)=res(j,i);
    return res;
}



const Eigen::MatrixXd timeseries::corr() const
{
    Eigen::MatrixXd c = cov();
    Eigen::MatrixXd ret(c.rows(),c.cols());
    for (long i=0; i<c.rows(); i++)
        for (long j=0; j<c.cols(); j++)
            ret(i,j)=c(i,j)/(sqrt(c(i,i))*sqrt(c(j,j)));
    return ret;
}

/*
    string timet2string(const time_t  t) {
        char mbstr[100];
        time_t t1=t;
        if (std::strftime(mbstr, sizeof(mbstr), "%Y%m%d.%H%M%S", std::localtime(&t1)))
        return string(mbstr);
        else THROW_EXCEPTION("cannot convert "<<t);
    }
*/
ostream& operator << (ostream& os,const timeseries&  ts)
{
    char mbstr[100];
    char delimiter='\t';

    os << "\ndate\t";
    for (std::vector<string >::size_type i=0; i<ts.names.size(); i++)
        os << ts.names.at(i) <<delimiter;
    os << "\n";
    for (long i=0; i<ts.M.rows(); i++)
    {
        if (std::strftime(mbstr, sizeof(mbstr), "%Y%m%d.%H%M%S", std::localtime(&ts.dates.at(i))))
        {
            os<< mbstr << delimiter;
        }
        for (long j=0; j<ts.M.cols(); j++) os<< fixed<<ts.M(i,j)<<delimiter;
        os << "\n";
    }
    os << "\n";

    return os;
}


void timeseries::plot(size_t serie) const
{
//install plplot12-driver-xwin e libplplot-dev

    PLFLT xmin, xmax, ymin, ymax;
    PLFLT *x,*y;
    //const vector<time_t> t=ts.getDateTime();
    size_t len=getLength();
    size_t slen=getSeriesLen();
    if (serie>=slen) THROW_EXCEPTION("index out of bound :"<<serie);
    if (slen<1 || len<1) THROW_EXCEPTION("empty timeseries");
    //timeseries::freq_enum f= getFreq();
    plstream *pls = new plstream();
    pls->sdev((char *)"xwin");//bisogna installare plplot xwin drivers !!!
    pls->init();
    if (this->freq==timeseries::freq_enum::daily || this->freq==timeseries::freq_enum::weekly)
        pls->timefmt("%y%m%d");
    else pls->timefmt("%y%m");

    x=new PLFLT[len];
    y=new PLFLT[len];
    for (size_t i=0; i<len; i++)
    {
        x[i]= this->dates[i];//ts.getDateTime(i);
        y[i]= this->M(i,serie);// ts(i,serie);//first ts
        if (i==0)
        {
            xmin=xmax=x[i];
            ymin=ymax=y[i];
        }
        else
        {
            if (x[i]<xmin) xmin=x[i];
            if (x[i]>xmax) xmax=x[i];
            if (y[i]<ymin) ymin=y[i];
            if (y[i]>ymax) ymax=y[i];
        }
    }

    pls->env(xmin,xmax,ymin,ymax,0,40);
    pls->lab("time","values",(this->names[serie]).c_str());
    pls->line(len, x, y);
    delete x;
    delete y;
    delete pls;

}

string timeseries::date2string(time_t date)
{
    char mbstr[100];
    if (std::strftime(mbstr, sizeof(mbstr), "%Y%m%d%H%M%S", std::localtime(&date)))
        return string(mbstr);
    else THROW_EXCEPTION("cannot convert "<<date);
}

void timeseries::toCSV(std::string filename,std::string delimiter ) const
{

    ofstream os;
    os.open (filename);
    char mbstr[100];
    //write header
    os << "\"date\""<<delimiter;
    for (std::vector<string >::size_type i=0; i<names.size(); i++)
        if (i<(names.size()-1)  ) os << "\""<<names.at(i) <<"\""<<delimiter;
        else os << "\""<<names.at(i) <<"\"";
    os<<"\n";
    for (long i=0; i<M.rows(); i++)
    {

        if (std::strftime(mbstr, sizeof(mbstr), "%Y%m%d%H%M%S", std::localtime(&dates.at(i))))
        {
            os<< mbstr << delimiter;
        }
        else os<< "NODATE" << delimiter;
        for (long j=0; j<M.cols(); j++) if (j<(M.cols()-1) )os<< fixed<<M(i,j)<<delimiter;
            else os<< fixed<<M(i,j);
        os << "\n";
    }


    os.close();
}


Eigen::MatrixXd timeseries::max() const
{
    return M.colwise().maxCoeff();
}
Eigen::MatrixXd timeseries::min() const
{
    return M.colwise().minCoeff();
}
Eigen::MatrixXd timeseries::maxabs() const
{
    return M.cwiseAbs().colwise().maxCoeff();
}
Eigen::MatrixXd timeseries::minabs() const
{
    return M.cwiseAbs().colwise().minCoeff();
}

const bool timeseries::one_tail_t_test_corr(size_t i,size_t j, double confidence) const
{

    static const double student[12]= {0.679,0.849,1.047,1.299,1.676,2.009,2.109,2.403,2.678,2.937,3.261,3.496};
    size_t idx=4;
    if (confidence>=.25) idx=0;
    else if (confidence>=.20) idx=1;
    else if (confidence>=.15) idx=2;
    else if (confidence>=.10) idx=3;
    else if (confidence>=.05) idx=4;
    else if (confidence>=.025) idx=5;
    else if (confidence>=.02) idx=6;
    else if (confidence>=.01) idx=7;
    else if (confidence>=.005) idx=8;
    else if (confidence>=.0025) idx=9;
    else if (confidence>=.001) idx=10;
    else  idx=11;
    if ((long)i>=this->M.cols() || (long)j>=this->M.cols()) THROW_EXCEPTION("index out of bound");
    if (M.rows()<50) THROW_EXCEPTION("too few samples < 50");
    timeseries newts=this->getserie(i).merge(this->getserie(j));
    Eigen::MatrixXd c=newts.corr();
    double r=c(0,1);
    double r2=r*r;
    double t=r*sqrt( (newts.getLength()-2)/(1-r2) );
    return t>student[idx];

}
const bool timeseries::dateexists(time_t d) const
{
    return std::binary_search(dates.begin(),dates.end(),d);

}
const size_t timeseries::dateidx(time_t d) const
{
    vector<time_t>::const_iterator it=find(dates.begin(),dates.end(),d);
    if (it!=dates.end()) return (it-dates.begin()) ;
    else THROW_EXCEPTION("date "<<d <<" not found");
}



const double timeseries::pearsoncorr2(size_t xcol,size_t ycol) const
{
    int n = this->getLength();
    if (xcol>=this->getSeriesLen() || ycol>=this->getSeriesLen()) THROW_EXCEPTION("index out of range "<<xcol<<"\t"<<ycol);

    double result = 0;
    int i = 0;
    double xmean = 0;
    double ymean = 0;
    double v = 0;
    double x0 = 0;
    double y0 = 0;
    double s = 0;
    bool samex;// = new bool();
    bool samey;// = new bool();
    double xv = 0;
    double yv = 0;
    double t1 = 0;
    double t2 = 0;

    //
    // Special case
    //
    if (n <= 1)
    {
        result = 0;
        return result;
    }
    //
    xmean = 0;
    ymean = 0;
    samex = true;
    samey = true;
    x0 = M(0,xcol);//x[0];
    y0 = M(0,ycol);
    v = (double)1 / (double)n;
    for (i = 0; i <= n - 1; i++)
    {
        s = M(i,xcol);// x[i];
        samex = samex &&  (s == x0);
        xmean = xmean + s * v;
        s = M(i,ycol);//y[i];
        samey = samey && (s == y0);
        ymean = ymean + s * v;
    }
    if (samex | samey)
    {
        result = 0;
        return result;
    }
    s = 0;
    xv = 0;
    yv = 0;
    for (i = 0; i <= n - 1; i++)
    {
        t1 = M(i,xcol) /*x[i] */- xmean;
        t2 = M(i,ycol) /*y[i] */- ymean;
        xv = xv + t1*t1;
        yv = yv + t2*t2;
        s = s + t1 * t2;
    }
    if ( (xv == 0)  || (yv== 0))
    {
        result = 0;
    }
    else
    {
        result = s / (sqrt(xv) * sqrt(yv));
    }
    return result;
}

const bool timeseries::timeseriefilter(size_t min_len,double maxtimegapinsec, double maxvaluegappct, double maxtimebefornowinsec) const {
    if (this->getLength()<2) THROW_EXCEPTION("length < 2");
    if (this->getLength()<min_len) return false;
    if (this->getDateGapInSeconds()>maxtimegapinsec) return false;
    if (difftime(time(0),this->getLastDate())>maxtimebefornowinsec) return false;
    for (size_t i=1;i<M.rows();i++)
        for (size_t j=0;j<M.cols();j++)
            if ( 100*fabs( (M(i,j)-M(i-1,j))/M(i-1,j) ) > fabs(maxvaluegappct) )return false;
        return true;
}
