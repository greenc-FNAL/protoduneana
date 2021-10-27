////////////////////////////////////////////////////////////////////////
// Class:       EdepCal
// Module Type: analyzer
// File:        EdepCal_module.cc
//
//
//If you have some questions you can write to dorota.stefan@cern.ch
//or robert.sulej@cern.ch  
//
////////////////////////////////////////////////////////////////////////

#include "lardataobj/Simulation/SimChannel.h"
#include "larsim/Simulation/LArG4Parameters.h"
#include "larsim/Simulation/LArVoxelData.h"
#include "larsim/Simulation/LArVoxelList.h"
#include "larsim/Simulation/SimListUtils.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h"
#include "lardata/Utilities/DatabaseUtil.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "TH1.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#include <cmath>

namespace proto
{
	struct bHitInfo;
	class EdepCal;
}

struct proto::bHitInfo
{
	bHitInfo(size_t i, double x, double e, int w) :
		Index(i), dE(e), dx(x), wire(w)
	{ }
	size_t Index;
	double dE, dx;
	int wire;
};

class proto::EdepCal : public art::EDAnalyzer {
public:
  	explicit EdepCal(fhicl::ParameterSet const & p);
  	// The destructor generated by the compiler is fine for classes
  	// without bare pointers or other resource use.

  	// Plugins should not be copied or assigned.
  	EdepCal(EdepCal const &) = delete;
  	EdepCal(EdepCal &&) = delete;
  	EdepCal & operator = (EdepCal const &) = delete;
  	EdepCal & operator = (EdepCal &&) = delete;

  	// Required functions.
	void analyze(art::Event const & e) override;

	void beginJob() override;

	void beginRun(const art::Run& run) override;
	
	void reconfigure(fhicl::ParameterSet const& p) ;
private:

  // Declare member data here.
	void ResetVars();

	geo::GeometryCore const * fGeometry;
	double fElectronsToGeV;

	bool Has(std::vector<int> v, int i) const;

	double GetEdepMC(art::Event const & e) const;
	
	double GetEdepAttenuatedMC(art::Event const & e) const;
	
	double GetEdepEM_MC(art::Event const & e) const;
	
	double GetEdepEMAttenuated_MC(art::Event const & e) const;

//	double GetEdepTotVox(art::Event const & e) const;

        double GetEdepHits(detinfo::DetectorClocksData const& clockData,
                           detinfo::DetectorPropertiesData const& detProp,
                           const std::vector< recob::Hit > & hits) const;
	
        double GetEdepHits(detinfo::DetectorClocksData const& clockData,
                           detinfo::DetectorPropertiesData const& detProp,
                           const std::vector< art::Ptr<recob::Hit> > & hits) const;
	
	double GetEdepHitsMeV(const std::vector< recob::Hit > & hits) const;
	
	int fBestview;

	double fT0;
	
	//////
	TTree *fTree;
	int fRun; 
	int fEvent;
	double fEnGen;
	double fEkGen;
	double fEdep;
	double fEdepMeV; 
	double fEdepCl;
	double fEdepMC;
	double fEdepAttMC;
//	double fEdepMCTotV;
	double fEdepEMMC;
	double fEdepEMAttMC;
	double fRatioTot;
	double fRatioEM;
	double fRatioHad;
	//////

	//std::vector< art::Ptr<simb::MCParticle> > fSimlist;
	//std::vector< art::Ptr<recob::Hit> > fHitlist;
	//std::vector< art::Ptr<recob::Cluster> > fClusterlist;

	// Module labels to get data products
	std::string fSimulationLabel;
	std::string fHitsModuleLabel;
	std::string fClusterModuleLabel;

	calo::CalorimetryAlg fCalorimetryAlg;
	
	std::unordered_map< int, const simb::MCParticle* > fParticleMap;
};

proto::EdepCal::EdepCal(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p),
	fCalorimetryAlg(p.get<fhicl::ParameterSet>("CalorimetryAlg"))
 // More initializers here.
{
	// get a pointer to the geometry service provider
	fGeometry = &*(art::ServiceHandle<geo::Geometry>());
	
	reconfigure(p);
}

void proto::EdepCal::beginRun(const art::Run&)
{
	art::ServiceHandle<sim::LArG4Parameters> larParameters;
  	fElectronsToGeV = 1./larParameters->GeVToElectrons();
}

void proto::EdepCal::beginJob()
{
	// access art's TFileService, which will handle creating and writing hists
	art::ServiceHandle<art::TFileService> tfs;

	fTree = tfs->make<TTree>("calibration","calibration tree");
	fTree->Branch("fRun", &fRun, "fRun/I");
	fTree->Branch("fEvent", &fEvent, "fEvent/I");
	fTree->Branch("fEnGen", &fEnGen, "fEnGen/D");
	fTree->Branch("fEkGen", &fEkGen, "fEkGen/D");
	fTree->Branch("fEdep", &fEdep, "fEdep/D");
	fTree->Branch("fEdepMeV", &fEdepMeV, "fEdepMeV/D");
	fTree->Branch("fEdepCl", &fEdepCl, "fEdepCl/D");
	fTree->Branch("fEdepMC", &fEdepMC, "fEdepMC/D");
	fTree->Branch("fEdepAttMC", &fEdepAttMC, "fEdepAttMC/D");
//	fTree->Branch("fEdepMCTotV", &fEdepMCTotV, "fEdepMCTotV/D");
	fTree->Branch("fEdepEMMC", &fEdepEMMC, "fEdepEMMC/D");
	fTree->Branch("fEdepEMAttMC", &fEdepEMAttMC, "fEdepEMAttMC/D");
	fTree->Branch("fRatioTot", &fRatioTot, "fRatioTot/D");
	fTree->Branch("fRatioEM", &fRatioEM, "fRatioEM/D");
	fTree->Branch("fRatioHad", &fRatioHad, "fRatioHad/D");
}

void proto::EdepCal::reconfigure(fhicl::ParameterSet const & p)
{
	fSimulationLabel = p.get< std::string >("SimulationLabel");
	fHitsModuleLabel = p.get< std::string >("HitsModuleLabel");
	fClusterModuleLabel = p.get< std::string >("ClusterModuleLabel");

	fBestview = p.get<int>("Bestview");
}

void proto::EdepCal::analyze(art::Event const & e)
{
 	// Implementation of required member function here.
	ResetVars();

	fRun = e.run();
	fEvent = e.id().event();
	
	// MC particle list
	auto particleHandle = e.getValidHandle< std::vector<simb::MCParticle> >(fSimulationLabel);	
	bool flag = true;
	
	for (auto const& p : *particleHandle)
	{
		fParticleMap[p.TrackId()] = &p;
		if ((p.Process() == "primary") && flag)
		{
			fEnGen = p.P();
			fEkGen = (std::sqrt(p.P()*p.P() + p.Mass()*p.Mass()) - p.Mass()) * 1000; // MeV
			fT0 = p.T();
			flag = false;	
		}
	}

	// MC
	fEdepMC = GetEdepMC(e);
	fEdepAttMC = GetEdepAttenuatedMC(e);
//	fEdepMCTotV = GetEdepTotVox(e);
	fEdepEMMC = GetEdepEM_MC(e);
	fEdepEMAttMC = GetEdepEMAttenuated_MC(e);
	
	//sim::IDE
	//auto simchannel = e.getValidHandle< std::vector<sim::SimChannel> >(fSimulationLabel);
	
	// hits
	fEdep = 0.0;
	const auto& hitListHandle = *e.getValidHandle< std::vector<recob::Hit> >(fHitsModuleLabel);

        auto const clockData = art::ServiceHandle<detinfo::DetectorClocksService>()->DataFor(e);
        auto const detProp = art::ServiceHandle<detinfo::DetectorPropertiesService>()->DataFor(e, clockData);
        fEdep = GetEdepHits(clockData, detProp, hitListHandle);
	fEdepMeV = GetEdepHitsMeV(hitListHandle);

	// clusters
	auto clListHandle = e.getValidHandle< std::vector<recob::Cluster> >(fClusterModuleLabel);

	// look for all hits associated to selected cluster
	art::FindManyP< recob::Hit > hitsFromClusters(clListHandle, e, fClusterModuleLabel);

	fEdepCl = 0.0;
	for (size_t c = 0; c < clListHandle->size(); ++c)
	{
                fEdepCl += GetEdepHits(clockData, detProp, hitsFromClusters.at(c));
	}
	
	
	if (fEdepMC > 0.0)
	{
		fRatioTot = fEdep / fEdepMC;
	}
	if (fEdepEMMC > 0.0)
	{
		fRatioEM = fEdepCl / fEdepEMMC;
	}
	
	double edephad = fEdepMC - fEdepEMMC;
	if (edephad > 0)
	{
		fRatioHad = (fEdep - fEdepCl) / edephad;
	}

	fTree->Fill();
}

double proto::EdepCal::GetEdepMC(art::Event const & e) const
{
	double energy = 0.0;

	auto simchannelHandle = e.getHandle< std::vector<sim::SimChannel> >(fSimulationLabel);
	if (simchannelHandle)
	{
			for ( auto const& channel : (*simchannelHandle) )
			{
				if (fGeometry->View(channel.Channel()) == fBestview) 
				{
					// for every time slice in this channel:
					auto const& timeSlices = channel.TDCIDEMap();
					for ( auto const& timeSlice : timeSlices )
					{
						// loop over the energy deposits.
						auto const& energyDeposits = timeSlice.second;
		
						for ( auto const& energyDeposit : energyDeposits )
						{
							energy += energyDeposit.energy;
						}
					}
				}
			}
	}

	return energy;
}

double proto::EdepCal::GetEdepAttenuatedMC(art::Event const & e) const
{
	double energy = 0.0;

	auto simchannelHandle = e.getHandle< std::vector<sim::SimChannel> >(fSimulationLabel);
	if (simchannelHandle)
	{
			for ( auto const& channel : (*simchannelHandle) )
			{
				if (fGeometry->View(channel.Channel()) == fBestview) 
				{
					// for every time slice in this channel:
					auto const& timeSlices = channel.TDCIDEMap();
					for ( auto const& timeSlice : timeSlices )
					{
						// loop over the energy deposits.
						auto const& energyDeposits = timeSlice.second;
		
						for ( auto const& energyDeposit : energyDeposits )
						{
							energy += energyDeposit.numElectrons * fElectronsToGeV * 1000;
						}
					}
				}
			}
	}
	
	return energy;
		                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
}

double proto::EdepCal::GetEdepEM_MC(art::Event const & e) const
{
	double enEM = 0.0;
	
	auto simchannelHandle = e.getHandle< std::vector<sim::SimChannel> >(fSimulationLabel);
	if (simchannelHandle)
	{
			for ( auto const& channel : (*simchannelHandle) )
			{
				if (fGeometry->View(channel.Channel()) == fBestview)
				{ 
					// for every time slice in this channel:
					auto const& timeSlices = channel.TDCIDEMap();
					for ( auto const& timeSlice : timeSlices )
					{
						// loop over the energy deposits.
						auto const& energyDeposits = timeSlice.second;
		
						for ( auto const& energyDeposit : energyDeposits )
						{
							double energy = energyDeposit.energy;
							int trackID = energyDeposit.trackID;
							
							if (trackID < 0)
							{
								enEM += energy;
							}
							else if (trackID > 0)
							{
								auto search = fParticleMap.find(trackID);
								bool found = true;
								if (search == fParticleMap.end())
								{
									mf::LogWarning("TrainingDataAlg") << "PARTICLE NOT FOUND";
									found = false;
								}
								
								int pdg = 0;
								if (found)
								{
									const simb::MCParticle& particle = *((*search).second);
                                                			if (!pdg) pdg = particle.PdgCode(); // not EM activity so read what PDG it is
								}
								
								if ((pdg == 11) || (pdg == -11) || (pdg == 22)) enEM += energy;
							}
							
						}
					}
				}
			}
	}
	return enEM;
}

double proto::EdepCal::GetEdepEMAttenuated_MC(art::Event const & e) const
{
	double enEM = 0.0;


	auto simchannelHandle = e.getHandle< std::vector<sim::SimChannel> >(fSimulationLabel);
	if (simchannelHandle)
	{
			for ( auto const& channel : (*simchannelHandle) )
			{
				if (fGeometry->View(channel.Channel()) == fBestview)
				{
					// for every time slice in this channel:
					auto const& timeSlices = channel.TDCIDEMap();
					for ( auto const& timeSlice : timeSlices )
					{
						// loop over the energy deposits.
						auto const& energyDeposits = timeSlice.second;
		
						for ( auto const& energyDeposit : energyDeposits )
						{
							double energy = energyDeposit.numElectrons * fElectronsToGeV * 1000;
							int trackID = energyDeposit.trackID;
							
							if (trackID < 0)
							{
								enEM += energy;
							}
							else if (trackID > 0)
							{
								auto search = fParticleMap.find(trackID);
								bool found = true;
								if (search == fParticleMap.end())
								{
									mf::LogWarning("TrainingDataAlg") << "PARTICLE NOT FOUND";
									found = false;
								}
								
								int pdg = 0;
								if (found)
								{
									const simb::MCParticle& particle = *((*search).second);
                                                			if (!pdg) pdg = particle.PdgCode(); // not EM activity so read what PDG it is
								}
								
								if ((pdg == 11) || (pdg == -11) || (pdg == 22)) enEM += energy;
							}
							
						}
					}
				}
			}
	}
	return enEM;
}
// before recombination takes place and other attenuations
/*double proto::EdepCal::GetEdepTotVox(art::Event const & e) const
{
	double en = 0.0;

	const sim::LArVoxelList voxels = sim::SimListUtils::GetLArVoxelList(e, fSimulationLabel);

	sim::LArVoxelList::const_iterator vxitr;
	for (vxitr = voxels.begin(); vxitr != voxels.end(); vxitr++)
	{
		const sim::LArVoxelData &vxd = (*vxitr).second;
		for (size_t partIdx = 0; partIdx < vxd.NumberParticles(); partIdx++)
		{
			en += vxd.Energy(partIdx);
		}
	}
	
	return en;
}*/

double proto::EdepCal::GetEdepHits(detinfo::DetectorClocksData const& clockData,
                                   detinfo::DetectorPropertiesData const& detProp,
                                   const std::vector< recob::Hit > & hits) const
{
	if (!hits.size()) return 0.0;

	double dqsum = 0.0;
	for (size_t h = 0; h < hits.size(); ++h)
	{
		unsigned short plane = hits[h].WireID().Plane;
		if (plane != fBestview) continue;

		double dqadc = hits[h].Integral();
		if (!std::isnormal(dqadc) || (dqadc < 0)) continue;
	
		double dqel = fCalorimetryAlg.ElectronsFromADCArea(dqadc, plane);
		
		double tdrift = hits[h].PeakTime();
                double correllifetime = fCalorimetryAlg.LifetimeCorrection(clockData, detProp, tdrift, fT0);

		double dq = dqel * correllifetime * fElectronsToGeV * 1000;
		if (!std::isnormal(dq) || (dq < 0)) continue;

		dqsum += dq; 
	}

	return dqsum; 
}

double proto::EdepCal::GetEdepHitsMeV(const std::vector< recob::Hit > & hits) const
{
	if (!hits.size()) return 0.0;

	double dqsum = 0.0;
	for (size_t h = 0; h < hits.size(); ++h)
	{
		unsigned short plane = hits[h].WireID().Plane;
		if (plane != fBestview) continue;

		double dqadc = hits[h].Integral();
		if (!std::isnormal(dqadc) || (dqadc < 0)) continue;
	
		double dqel = fCalorimetryAlg.ElectronsFromADCArea(dqadc, plane);

		double dq = dqel * fElectronsToGeV * 1000;
		if (!std::isnormal(dq) || (dq < 0)) continue;

		dqsum += dq; 
	}

	return dqsum; 
}

double proto::EdepCal::GetEdepHits(detinfo::DetectorClocksData const& clockData,
                                   detinfo::DetectorPropertiesData const& detProp,
                                   const std::vector< art::Ptr<recob::Hit> > & hits) const
{
	if (!hits.size()) return 0.0;

	double dqsum = 0.0;
	for (size_t h = 0; h < hits.size(); ++h)
	{
		unsigned short plane = hits[h]->WireID().Plane;
		if (plane != fBestview) continue;
	
		double dqadc = hits[h]->Integral();
		if (!std::isnormal(dqadc) || (dqadc < 0)) continue;
	
		double dqel = fCalorimetryAlg.ElectronsFromADCArea(dqadc, plane);
		
		double tdrift = hits[h]->PeakTime();
                double correllifetime = fCalorimetryAlg.LifetimeCorrection(clockData, detProp, tdrift, fT0);

		double dq = dqel * correllifetime * fElectronsToGeV * 1000;
		if (!std::isnormal(dq) || (dq < 0)) continue;

		dqsum += dq; 
	}
	
	return dqsum; 
}

bool proto::EdepCal::Has(std::vector<int> v, int i) const
{
	for (auto c : v) if (c == i) return true;
  	return false;
}


void proto::EdepCal::ResetVars()
{
	fEdep = 0.0;
	fEdepMeV = 0.0;
	fEnGen = 0.0;
	fEkGen = 0.0;
	fEdepCl = 0.0;
	fEdepMC = 0.0;
	fEdepAttMC = 0.0;
//	fEdepMCTotV = 0.0;
	fEdepEMMC = 0.0;
	fEdepEMAttMC = 0.0;
	fRatioTot = 0.0;
	fRatioEM = 0.0;
	fRatioHad = 0.0;
	fT0 = 0.0;
	fParticleMap.clear();
}

DEFINE_ART_MODULE(proto::EdepCal)
