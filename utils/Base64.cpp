#include "Base64.h"

namespace RepoSync
{
    std::string Base64Decode(const std::string& input)
    {
        static const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string clean;
        clean.reserve(input.size());

        for (char c : input)
        {
            if (c != '\n' && c != '\r' && c != ' ')
                clean.push_back(c);
        }

        std::string output;

        int val = 0;
        int valb = -8;

        for (unsigned char c : clean)
        {
            if (c == '=')
                break;

            int index = static_cast<int>(chars.find(c));
            if (index == static_cast<int>(std::string::npos))
                break;

            val = (val << 6) + index;
            valb += 6;

            if (valb >= 0)
            {
                output.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        return output;
    }
}