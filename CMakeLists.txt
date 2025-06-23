add_llvm_pass_plugin (
    MyHelloPass
    PassPluginInfo.cpp 

    FunctionAnalyzer.hpp
    FunctionAnalyzer.cpp

    BlockClass.hpp
    BlockClass.cpp

    RangeHandler.hpp
    RangeHandler.cpp

    ScopeHandler.hpp
    ScopeHandler.cpp

    InstructionAnalyzer.hpp
    InstructionAnalyzer.cpp
    
    VRAPass.h 
    VRAPass.cpp

    BlockVisitor.hpp
    RangePropagationVisitor.cpp
    
    Utils.hpp
    Utils.cpp)

