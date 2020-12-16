/*
 * ******************************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH 
 * *****************************************************************************
 */


/** This will hold a VSS path and provide three representations
 *    - a VISS GEN2 path, i.e. Vehicle/Speed
 *    - a VISS GEN1 path, i.e. Vehicle.Speed
 *    - a JSON path, i.e $['Vehicle']['Speed']
 */
  
#ifndef __VSSPATH_HPP__
#define __VSSPATH_HPP__

#include <string>

class VSSPath {
    public:
        std::string getVSSPath();
        std::string getVSSGen1Path();
        std::string getJSONPath();
        bool isGen1Origin();
        static VSSPath fromVSS(std::string vss); //Expect Gen2 path
        static VSSPath fromVSSGen1(std::string vssgen1); //Expect Gen1 path
        static VSSPath fromJSON(std::string json); //Expect json path
        static VSSPath fromVSSAuto(std::string vss); //Auto decide for Gen1 or Gen2

    private:
        std::string vsspath;
        std::string vssgen1path;
        std::string jsonpath;
        bool gen1;

        static std::string gen1togen2(std::string input);
        static std::string gen2togen1(std::string input);
        static std::string gen2tojson(std::string input);
        static std::string jsontogen2(std::string input);
        

        VSSPath(std::string vss, std::string vssgen1, std::string jsonpath, bool gen1origin);
};

#endif