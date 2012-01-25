SET(TMENGINE_H
        TonemapOperatorAshikhmin02.h
        TonemapOperatorDrago03.h
        TonemapOperatorDurand02.h
        TonemapOperatorFattal02.h
        TonemapOperatorMantiuk06.h
        TonemapOperatorMantiuk08.h
        TonemapOperatorPattanaik00.h
        TonemapOperatorReinhard02.h
        TonemapOperatorReinhard05.h
        TonemapOperator.h
)

SET(TMENGINE_CPP
        TonemapOperatorAshikhmin02.cpp
        TonemapOperatorDrago03.cpp
        TonemapOperatorDurand02.cpp
        TonemapOperatorFattal02.cpp
        TonemapOperatorMantiuk06.cpp
        TonemapOperatorMantiuk08.cpp
        TonemapOperatorPattanaik00.cpp
        TonemapOperatorReinhard02.cpp
        TonemapOperatorReinhard05.cpp
        TonemapOperator.cpp
)

ADD_LIBRARY(tmengine ${TMENGINE_H} ${TMENGINE_CPP})
