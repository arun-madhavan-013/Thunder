#ifndef TIMEDATE_ADMINISTRATOR_H
#define TIMEDATE_ADMINISTRATOR_H

#include "Definitions.h"
#include "Descriptors.h"
#include "TDT.h"
#include "TunerAdministrator.h"

namespace WPEFramework {

namespace Broadcast {

    class TimeDate {
    private:
        TimeDate(const TimeDate&) = delete;
        TimeDate& operator=(const TimeDate&) = delete;

        class Sink : public TunerAdministrator::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            Sink(TimeDate& parent)
                : _parent(parent)
            {
            }
            virtual ~Sink()
            {
            }

        public:
            virtual void Activated(ITuner* tuner) override
            {
            }
            virtual void Deactivated(ITuner* tuner) override
            {
                _parent.Deactivated(tuner);
            }
            virtual void StateChange(ITuner* tuner) override
            {
                _parent.StateChange(tuner);
            }

        private:
            TimeDate& _parent;
        };

        class Parser : public ISection {
        private:
            Parser() = delete;
            Parser(const Parser&) = delete;
            Parser& operator=(const Parser&) = delete;

        public:
            Parser(TimeDate& parent, ITuner* source, const bool scan)
                : _parent(parent)
                , _source(source)
            {
                if (scan == true) {
                    Scan(true);
                }
            }
            virtual ~Parser()
            {
                Scan(false);
            }
            inline bool operator==(const ITuner* rhs) const
            {
                return (_source == rhs);
            }
            inline bool operator!=(const ITuner* rhs) const
            {
                return (!operator==(rhs));
            }

        public:
            void Scan(const bool scan)
            {
                if (scan == true) {
                    // Start loading the SDT info
                    _source->Filter(0x14, DVB::TDT::ID, this);
                    _source->Filter(0x14, DVB::TOT::ID, this);
                } else {
                    _source->Filter(0x14, DVB::TOT::ID, nullptr);
                    _source->Filter(0x14, DVB::TDT::ID, nullptr);
                }
            }

        private:
            virtual void Handle(const MPEG::Section& section) override
            {

                ASSERT(section.IsValid());

                if (section.TableId() == DVB::TDT::ID) {
                    _parent.Load(DVB::TDT(section));
                } else if (section.TableId() == DVB::TOT::ID) {
                    _parent.Load(DVB::TOT(section));
                }
            }

        private:
            TimeDate& _parent;
            ITuner* _source;
        };

        typedef std::list<Parser> Scanners;

    public:
        TimeDate()
            : _adminLock()
            , _scanners()
            , _sink(*this)
            , _scan(true)
            , _time()
        {
            TunerAdministrator::Instance().Register(&_sink);
        }
        virtual ~TimeDate()
        {
            TunerAdministrator::Instance().Unregister(&_sink);
        }

    public:
        void Scan(const bool scan)
        {
            _adminLock.Lock();

            if (_scan != scan) {
                _scan = scan;
                Scanners::iterator index(_scanners.begin());
                while (index != _scanners.end()) {
                    index->Scan(_scan);
                    index++;
                }
            }
            _adminLock.Unlock();
        }

    private:
        void Deactivated(ITuner* tuner)
        {
            _adminLock.Lock();

            Scanners::iterator index = std::find(_scanners.begin(), _scanners.end(), tuner);

            if (index != _scanners.end()) {
                _scanners.erase(index);
            }

            _adminLock.Unlock();
        }
        void StateChange(ITuner* tuner)
        {

            _adminLock.Lock();

            Scanners::iterator index = std::find(_scanners.begin(), _scanners.end(), tuner);

            if (index != _scanners.end()) {
                if (tuner->State() == ITuner::IDLE) {
                    _scanners.erase(index);
                }
            } else {
                if (tuner->State() != ITuner::IDLE) {
                    _scanners.emplace_back(*this, tuner, _scan);
                }
            }

            _adminLock.Unlock();
        }
        void Load(const DVB::TOT& table)
        {

            _adminLock.Lock();

            MPEG::DescriptorIterator index = table.Descriptors();

            while (index.Next() == true) {
            }

            _time = table.Time();

            _adminLock.Unlock();
        }
        void Load(const DVB::TDT& table)
        {

            _adminLock.Lock();

            _time = table.Time();

            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Scanners _scanners;
        Sink _sink;
        bool _scan;
        Core::Time _time;
    };

} // namespace Broadcast
} // namespace WPEFramework

#endif // TIMEDATE_ADMINISTRATOR_H
