#pragma once

#include <gtirb/CFGNodeInfo.hpp>

namespace gtirb
{
    ///
    /// \class CFGNodeInfoEntry
    /// \author John E. Farrier
    ///
    class GTIRB_GTIRB_EXPORT_API CFGNodeInfoActualIn : public CFGNodeInfo
    {
    public:
        ///
        /// Defaulted trivial destructor.
        ///
        virtual ~CFGNodeInfoActualIn() = default;

        ///
        /// Serialization support.
        ///
        template <class Archive>
        void serialize(Archive& ar, const unsigned int)
        {
            ar& boost::serialization::base_object<CFGNodeInfo>(*this);
        }

    private:
    };
}

BOOST_CLASS_EXPORT_KEY(gtirb::CFGNodeInfoActualIn);