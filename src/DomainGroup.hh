#ifndef __DOMAIN_GROUP_HH__
#define __DOMAIN_GROUP_HH__

#include <list>

#include "common.hh"
#include "FaultDomain.hh"

/**
 * 
 *  @brief Parent of DomainGroupDDR.
 *         Carry all channel object(FaultDomain) by <list>
 * 
 *  @param list FDList : carry all  channel object
 * 
 *  @fn double getFaultRate() : Get total fault rate of whole system.
 *  @fn void setInherent() : Alloacte the same kind of fault to all channel object's inherentfault(member variable)
 *  @fn setInitialRetiredBlkCount() : 
 * 
 * 
 *  @fn pickRandomFD() : Return random one channel object of system.
 * 
 *  @fn void scrub() : Do HW scrub to all channel objects
 *  @fn void clear() : Emtpty <list>
 *  @fn FaultDomain* getFd() : Return the first domain in the list.
 *  
 * 
 */ 
class DomainGroup {     // Corresponds to a cluster
public:
    DomainGroup() {}
    ~DomainGroup() {
        for (auto it=FDList.begin(); it!=FDList.end(); ++it) {
            delete *it;
        }
        FDList.clear();
    }

    double getFaultRate() { return (FDList.size()==0) ? 0 : FDList.size() * FDList.front()->getFaultRate(); }
    void setInherentFault(Fault *fault) { for (auto it=FDList.begin(); it!=FDList.end(); ++it) { (*it)->setInherentFault(fault); } }
    void setInitialRetiredBlkCount(ECC *ecc) { for (auto it=FDList.begin(); it!=FDList.end(); ++it) { (*it)->setInitialRetiredBlkCount(ecc); } }

    FaultDomain *pickRandomFD();

    void scrub() { for (auto it=FDList.begin(); it!=FDList.end(); ++it) { (*it)->scrub(); } }
    void clear() { for (auto it=FDList.begin(); it!=FDList.end(); ++it) { (*it)->clear(); } }
    FaultDomain *getFD() { return FDList.front(); }

protected:
    std::list<FaultDomain *> FDList;
};




/**
 *  @brief Define System configuration and initializae FaultDomainList for each Domain.
 *         With initializing by constructor, Put a lot of channel objects into the <list>
 * 
 *  @param domainsPerGroup (int) equals to number of channels in system
 *  @param ranksPerDomain  (int) equals to number of ranks in each channel
 *  @param devicesPerRank  (int) title soon content
 *  @param pinsPerDevice   (int) title soon content
 *  @param blkHeight       (int) Unit to check by ECC.
 * 
 * 
 */
class DomainGroupDDR : public DomainGroup {
public:
    DomainGroupDDR(int domainsPerGroup, int ranksPerDomain, int devicesPerRank, int pinsPerDevice, int blkHeight) {
        for (int i=0; i<domainsPerGroup; i++) {
            FDList.push_back(new FaultDomainDDR(ranksPerDomain, devicesPerRank, pinsPerDevice, blkHeight));
        }
    }
};

#endif /* __DOMAIN_GROUP_HH__ */
