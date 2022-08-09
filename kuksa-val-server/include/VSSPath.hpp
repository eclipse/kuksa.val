/**********************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH
 **********************************************************************/


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
        std::string getVSSPath() const;
        std::string getVSSGen1Path() const;
        std::string getJSONPath() const;
        std::string to_string() const;
        bool isGen1Origin() const;
        static VSSPath fromVSSGen2(std::string vss); //Expect Gen2 path
        static VSSPath fromVSSGen1(std::string vssgen1); //Expect Gen1 path
        static VSSPath fromJSON(std::string json, bool gen1); //Expect json path
        static VSSPath fromVSS(std::string vss); //Auto decide for Gen1 or Gen2
        inline bool operator==(const VSSPath& other) { return (vsspath == other.vsspath); };

        inline bool operator< (const VSSPath& other) const {
          return vsspath < other.vsspath;
        }

    private:
        std::string vsspath;
        std::string vssgen1path;
        std::string jsonpath;
        bool gen1_;

        static std::string gen1togen2(std::string input);
        static std::string gen2togen1(std::string input);
        static std::string gen2tojson(std::string input);
        static std::string jsontogen2(std::string input);
        

        VSSPath(std::string vss, std::string vssgen1, std::string jsonpath, bool gen1origin);
    
    friend inline bool operator==(const VSSPath& lhs, const VSSPath& rhs) { return (lhs.vsspath == rhs.vsspath); };
    friend inline bool operator!=(const VSSPath& lhs, const VSSPath& rhs) { return !(lhs == rhs); };

};

//specialize std:hash for VSSPath so it can be used in eg unordered_map
namespace std {

  template <>
  struct hash<VSSPath>
  {
    std::size_t operator()(const VSSPath& vp) const
    {
      using std::size_t;
      using std::hash;
      using std::string;
      //based on VSSPAth equality, we really only need to hash vsspath
      return hash<std::string>()(vp.getVSSPath());

    }
  };

}

#endif
