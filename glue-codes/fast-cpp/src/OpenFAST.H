#ifndef OpenFAST_h
#define OpenFAST_h

#include "FAST_Library.h"
#include "sys/stat.h"
#include "math.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <set>
#include <map>
#include "dlfcn.h"
#define OMPI_SKIP_MPICXX
#define MPICH_SKIP_MPICXX
#include "mpi.h"
#include "SC.h"


namespace fast {

struct globTurbineDataType {
  int TurbID;
  std::string FASTInputFileName;
  std::string FASTRestartFileName;
  std::vector<double> TurbineBasePos;
  std::vector<double> TurbineHubPos;
  int numForcePtsBlade;
  int numForcePtsTwr;
};

enum ActuatorNodeType {
  HUB = 0,
  BLADE = 1,
  TOWER = 2,
  ActuatorNodeType_END
};

class fastInputs {

 public: 

  MPI_Comm comm;
  int nTurbinesGlob;  
  bool dryRun;
  bool debug; 
  double tStart;
  int nEveryCheckPoint;  
  double tMax;
  double dtFAST;  

  bool scStatus;
  std::string scLibFile;
  int numScInputs, numScOutputs;

  std::vector<globTurbineDataType>  globTurbineData;

  // Constructor 
  fastInputs() ;
 
  // Destructor
  ~fastInputs() {} ;

};


class OpenFAST {

 private:

  MPI_Comm mpiComm;
  bool dryRun;        // If this is true, class will simply go through allocation and deallocation of turbine data
  bool debug;   // Write out extra information if this flags is turned on
  std::vector<globTurbineDataType> globTurbineData;
  int nTurbinesProc;
  int nTurbinesGlob;
  bool restart;
  bool timeZero;
  double dtFAST;
  double tMax;
  std::vector<std::vector<float>> TurbineBasePos;
  std::vector<std::vector<float>> TurbineHubPos;
  std::vector<int> TurbID;
  std::vector<std::string > FASTInputFileName;
  std::vector<std::string > CheckpointFileRoot;
  double tStart;
  int nt_global;           
  int ntStart;      // The time step to start the FAST simulation
  int nEveryCheckPoint;    // Check point files will be written every 'nEveryCheckPoint' time steps
  std::vector<int> numBlades;           // Number of blades
  std::vector<int> numForcePtsBlade;
  std::vector<int> numForcePtsTwr;
  std::vector<int> numVelPtsBlade;
  std::vector<int> numVelPtsTwr;
  int numScOutputs;  // # outputs from the supercontroller == # inputs to the controller == NumSC2Ctrl
  int numScInputs;   // # inputs to the supercontroller == # outputs from the controller == NumCtrl2SC
  std::vector<double> scOutputsGlob;  // # outputs from the supercontroller for all turbines
  std::vector<double> scInputsGlob;   // # inputs to the supercontroller for all turbines

  std::vector<std::vector<std::vector<double>>> forceNodeVel; // Velocity at force nodes - Store temporarily to interpolate to the velocity nodes
  std::vector<OpFM_InputType_t> cDriver_Input_from_FAST;
  std::vector<OpFM_OutputType_t> cDriver_Output_to_FAST;

  std::vector<SC_InputType_t> cDriverSC_Input_from_FAST;
  std::vector<SC_OutputType_t> cDriverSC_Output_to_FAST;

  // Turbine Number is DIFFERENT from TurbID. Turbine Number simply runs from 0:n-1 locally and globally.
  std::map<int, int> turbineMapGlobToProc; // Mapping global turbine number to processor number
  std::map<int, int> turbineMapProcToGlob; // Mapping local to global turbine number
  std::map<int, int> reverseTurbineMapProcToGlob; // Reverse Mapping global turbine number to local turbine number
  std::set<int> turbineSetProcs; // Set of processors containing atleast one turbine 
  std::vector<int> turbineProcs; // Same as the turbineSetProcs, but as an integer array

  //Supercontroller stuff
  bool scStatus;
  std::string scLibFile;
  // Dynamic load stuff copied from 'C++ dlopen mini HOWTO' on tldp.org
  void *scLibHandle ; 
  typedef SuperController* create_sc_t(); 
  create_sc_t * create_SuperController;
  typedef void destroy_sc_t(SuperController *); 
  destroy_sc_t * destroy_SuperController;
  SuperController * sc;

  int fastMPIGroupSize;
  MPI_Group fastMPIGroup;
  MPI_Comm  fastMPIComm;
  int fastMPIRank;

  MPI_Group worldMPIGroup;
  int worldMPIRank;

  int ErrStat;
  char ErrMsg[INTERFACE_STRING_LENGTH];  // make sure this is the same size as IntfStrLen in FAST_Library.f90

 public: 

  // Constructor 
  OpenFAST() ;
  
  // Destructor
  ~OpenFAST() {} ;

  void setInputs(const fastInputs &);  

  void init();
  void solution0();
  void step();
  void end();

  void setTurbineProcNo(int iTurbGlob, int procNo) { turbineMapGlobToProc[iTurbGlob] = procNo; }
  void allocateTurbinesToProcsSimple();
  void getHubPos(std::vector<double> & currentCoords, int iTurbGlob);
  void getHubShftDir(std::vector<double> & hubShftVec, int iTurbGlob);

  ActuatorNodeType getVelNodeType(int iTurbGlob, int iNode);
  void getVelNodeCoordinates(std::vector<double> & currentCoords, int iNode, int iTurbGlob);
  void setVelocity(std::vector<double> & velocity, int iNode, int iTurbGlob);
  void interpolateVel_ForceToVelNodes();
  ActuatorNodeType getForceNodeType(int iTurbGlob, int iNode);
  void getForceNodeCoordinates(std::vector<double> & currentCoords, int iNode, int iTurbGlob);
  void getForceNodeOrientation(std::vector<double> & currentOrientation, int iNode, int iTurbGlob);
  void getForce(std::vector<double> & force, int iNode, int iTurbGlob);
  double getChord(int iNode, int iTurbGlob);

  int get_ntStart() { return ntStart; }
  bool isDryRun() { return dryRun; }
  bool isDebug() { return debug; }
  bool isRestart() { return restart; }
  bool isTimeZero() { return timeZero; }
  int get_procNo(int iTurbGlob) { return turbineMapGlobToProc[iTurbGlob] ; } // Get processor number of a turbine with global id 'iTurbGlob'
  int get_localTurbNo(int iTurbGlob) { return reverseTurbineMapProcToGlob[iTurbGlob]; }
  int get_nTurbinesGlob() { return nTurbinesGlob; } 

  int get_numBlades(int iTurbGlob) { return get_numBladesLoc(get_localTurbNo(iTurbGlob)); }
  int get_numVelPtsBlade(int iTurbGlob) { return get_numVelPtsBladeLoc(get_localTurbNo(iTurbGlob)); }
  int get_numVelPtsTwr(int iTurbGlob) { return get_numVelPtsTwrLoc(get_localTurbNo(iTurbGlob)); }
  int get_numVelPts(int iTurbGlob) { return get_numVelPtsLoc(get_localTurbNo(iTurbGlob)); }
  int get_numForcePtsBlade(int iTurbGlob) { return get_numForcePtsBladeLoc(get_localTurbNo(iTurbGlob)); }
  int get_numForcePtsTwr(int iTurbGlob) { return get_numForcePtsTwrLoc(get_localTurbNo(iTurbGlob)); }
  int get_numForcePts(int iTurbGlob) { return get_numForcePtsLoc(get_localTurbNo(iTurbGlob)); }

  void computeTorqueThrust(int iTurGlob, std::vector<double> &  torque, std::vector<double> &  thrust);

 private:

  void checkError(const int ErrStat, const char * ErrMsg);
  inline bool checkFileExists(const std::string& name);

  void allocateMemory();
  
  int get_numBladesLoc(int iTurbLoc) { return numBlades[iTurbLoc]; }
  int get_numVelPtsBladeLoc(int iTurbLoc) { return numVelPtsBlade[iTurbLoc]; }
  int get_numVelPtsTwrLoc(int iTurbLoc) { return numVelPtsTwr[iTurbLoc]; }
  int get_numVelPtsLoc(int iTurbLoc) { return 1 + numBlades[iTurbLoc]*numVelPtsBlade[iTurbLoc] + numVelPtsTwr[iTurbLoc]; }
  int get_numForcePtsBladeLoc(int iTurbLoc) { return numForcePtsBlade[iTurbLoc]; }
  int get_numForcePtsTwrLoc(int iTurbLoc) { return numForcePtsTwr[iTurbLoc]; }
  int get_numForcePtsLoc(int iTurbLoc) { return 1 + numBlades[iTurbLoc]*numForcePtsBlade[iTurbLoc] + numForcePtsTwr[iTurbLoc]; }

  void loadSuperController(const fastInputs & fi);
  void fillScInputsGlob() ;
  void fillScOutputsLoc() ;

  void setOutputsToFAST(OpFM_InputType_t cDriver_Input_from_FAST, OpFM_OutputType_t cDriver_Output_to_FAST) ; // An example to set velocities at the Aerodyn nodes

};

}

#endif