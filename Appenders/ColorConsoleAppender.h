#pragma once
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/WinApi.h>

namespace plog
{
    template<class Formatter>
    class PLOG_LINKAGE_HIDDEN ColorConsoleAppender : public ConsoleAppender<Formatter>
    {
    public:
#ifdef _WIN32
#   ifdef _MSC_VER
#       pragma warning(suppress: 26812) //  Prefer 'enum class' over 'enum'
#   endif
        ColorConsoleAppender(OutputStream outStream = streamStdOut)
            : ConsoleAppender<Formatter>(outStream)
            , m_originalAttr()
        {
            if (this->m_isatty)
            {
                CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
                GetConsoleScreenBufferInfo(this->m_outputHandle, &csbiInfo);

                m_originalAttr = csbiInfo.wAttributes;
            }
        }
#else
        ColorConsoleAppender(OutputStream outStream = streamStdOut)
            : ConsoleAppender<Formatter>(outStream)
        {}
#endif

        virtual void write(const Record& record) PLOG_OVERRIDE
        {
            util::nstring str = Formatter::format(record);
            util::MutexLock lock(this->m_mutex);

            setColor(record.getSeverity());
            this->writestr(str);
            resetColor();
        }

    protected:
        void setColor(Severity severity)
        {
            if (this->m_isatty)
            {
                switch (severity)
                {
#ifdef _WIN32
                case fatal:
                    SetConsoleTextAttribute(this->m_outputHandle,
                        plog::foreground::kRed | plog::foreground::kBlue | plog::foreground::kIntensity |
                        plog::background::kRed); // bright magenta on red bg
                    break;

                case error:
                    SetConsoleTextAttribute(this->m_outputHandle,
                        static_cast<WORD>(plog::foreground::kRed | plog::foreground::kIntensity | (m_originalAttr & 0xf0))); // hot pink / bold red
                    break;

                case warning:
                    SetConsoleTextAttribute(this->m_outputHandle,
                        static_cast<WORD>(plog::foreground::kRed | plog::foreground::kGreen | plog::foreground::kIntensity | (m_originalAttr & 0xf0))); // yellow-pink blend
                    break;

                case info:
                    SetConsoleTextAttribute(this->m_outputHandle,
                        static_cast<WORD>(plog::foreground::kRed | plog::foreground::kBlue | plog::foreground::kIntensity | (m_originalAttr & 0xf0))); // technoblade purple
                    break;

                case debug:
                    SetConsoleTextAttribute(this->m_outputHandle,
                        static_cast<WORD>(plog::foreground::kGreen | plog::foreground::kBlue | plog::foreground::kIntensity | (m_originalAttr & 0xf0))); // cyan
                    break;

                case verbose:
                    SetConsoleTextAttribute(this->m_outputHandle,
                        static_cast<WORD>(plog::foreground::kRed | plog::foreground::kBlue | (m_originalAttr & 0xf0))); // magenta dim
                    break;
#else
                case fatal:
                    this->m_outputStream << "\x1B[97m\x1B[41m"; // white on red background
                    break;

                case error:
                    this->m_outputStream << "\x1B[91m"; // red
                    break;

                case warning:
                    this->m_outputStream << "\x1B[93m"; // yellow
                    break;

                case debug:
                case verbose:
                    this->m_outputStream << "\x1B[96m"; // cyan
                    break;
#endif
                default:
                    break;
                }
            }
        }

        void resetColor()
        {
            if (this->m_isatty)
            {
#ifdef _WIN32
                SetConsoleTextAttribute(this->m_outputHandle, m_originalAttr);
#else
                this->m_outputStream << "\x1B[0m\x1B[0K";
#endif
            }
        }

    private:
#ifdef _WIN32
        WORD   m_originalAttr;
#endif
    };
}
