#ifndef SECURITY_H_INCLUDED
#define SECURITY_H_INCLUDED
#include <string>
#include <vector>
#include <map>
#include "timeseries.hpp"


class security
{
private:


    std::string name;


    const timeseries init(std::string dbname,std::string symbol,std::string market,timeseries::freq_enum f);

public:
    const timeseries ts;

    /**
    effettua una query sql "selectq" sul db "dbname" e restituisce un vettore di vettori (righe) di stringhe come risultato
    */
    static std::vector< std::vector<std::string> >  querydb(std::string dbname,std::string selectq);
    /**
        legge un file csv diviso da n=tokens parti, delimitato da 'delimiter' e restituisce un vettore di vettori (righe) di stringhe come risultato
        modello finanza libera values=name,freq,date,o,h,l,c,v,oi
    */
    static const std::vector< std::vector<std::string> > readCSV(std::string filename,unsigned tokens,char delimiter=',');

    /** crea un db con 2 tabelle, una per i dati 'securities' in formato name,freq,date,o,h,l,c,v,oi, un'altra 'SECINFO' in formato isin, name ,code ,sector
     * @param name db name
     * @return void
     */
    static void createDB(std::string name);

    /**
     * inserisce un record nella tabella SECINFO, se non si dispone dell'ISIN utilizzare l'md5 del nome
    */
    static void insert_db_SECINFO(std::string dbname,std::string isin,std::string name, std::string code,std::string sector,std::string currency,std::string market,std::string type, std::string ysym,std::string gsym) ;
    /**
        popola la tabella 'SECINFO' caricando i dati dal sito di borsa italiana
    */
    static void populate_db_SECINFO(std::string dbname);
    /**
        popola la tabella 'securities' tramite il vettore di vettori 'values'
        name,market,freq,date(YYYYMMDD),open,high,low,close,volume,openinterest
    */
    static void populate_db(std::string dbname,const std::vector< std::vector<std::string> >& values);
    /**
        scarica dati da yahoo per i simboli che hanno il flag yahoo
    */
    static void loadDailyQuotesYahoo(std::string db);
    /**
        scarica dati da google per i simboli che hanno il flag google
    */
    static void loadDailyQuotesGoogle(std::string db);
    /**
        scarica i dati eod da yahoo e google
    */
    static void populate_db(std::string dbname);//carica dati da yahoo e google

    /** crea un nuovo db scaricando i simboli ed i dati
    */
    static void initialize_db(std::string dbname);

    /**
        torna una hashmap delle coppie 'security,frequenza ' dal db
    */
    static const std::map<std::string,std::string> getSecFreqList(std::string dbname);
    /**
            fornisce la lista delle 'security' presenti nel DB
    */
    static const std::vector<std::string> getSecList(std::string dbname,std::string sector="",std::string currency="",std::string market="",std::string type="");
    /**
        costruttore security, carica dal db la security 'symbol' con frequenza 'freq'
    */
    security(std::string dbname,std::string symbol,std::string market,timeseries::freq_enum freq);
    const std::string getName() const;

    const double open(size_t i) const ;
    const double high(size_t i) const ;
    const double low(size_t i) const ;
    const double close(size_t i) const ;
    const double volume(size_t i) const ;
    const double oi(size_t i) const ;
    const double open(time_t i) const ;
    const double high(time_t i) const ;
    const double low(time_t i) const ;
    const double close(time_t i) const ;
    const double volume(time_t i) const ;
    const double oi(time_t i) const ;

};





#endif // SECURITY_H_INCLUDED
