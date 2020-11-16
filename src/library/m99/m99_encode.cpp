#include "./m99_encode.h"


namespace
{

    using namespace maniscalco;

    struct symbol_info
    {
        symbol_info(){}
        symbol_info(std::uint8_t symbol, std::uint32_t count):symbol_(symbol), count_(count){}
        std::uint8_t    symbol_;
        std::uint32_t   count_;
    };

    struct tiny_encode_table_entry_type
    {
        std::uint32_t value_;
        std::uint32_t length_;
    };

    using tiny_encode_table_type = tiny_encode_table_entry_type[8][8][8][8];


    tiny_encode_table_type tinyEncodeTable;
    auto const initialize = []
    (
        tiny_encode_table_type & result
    ) -> bool
    {
        for (std::uint32_t maxLeft = 0; maxLeft < 8; ++maxLeft)
        {
            for (std::uint32_t maxRight = 0; maxRight < 8; ++maxRight)
            {
                for (std::uint32_t left = 0; left <= maxLeft; ++left)
                {
                    for (std::uint32_t right = 0; right <= maxRight; ++right)
                    {
                        if ((maxLeft == 2) && (maxRight==2) && ((left+right)==1) && (left==1))
                            int y = 9;
                        std::uint32_t total = left + right;
                        if (total < 8)
                        {
                            std::uint32_t l = left;
                            std::uint32_t r = right;
                            std::uint32_t ml = maxLeft;
                            std::uint32_t mr = maxRight;
                            std::uint32_t t = (l + r);
                            if (t > ml)
                            {
                                std::uint32_t inferredRight = (t - ml);
                                mr -= inferredRight;
                                t -= inferredRight;
                            }
                            if (t > mr)
                            {
                                std::uint32_t inferredLeft = (t - mr);
                                l -= inferredLeft;
                                t -= inferredLeft;
                            }
                            std::uint32_t codeLength = 0;
                            while ((1ull << ++codeLength) <= t)
                                ;
                            --codeLength;
                            auto needMsb = ((l | (1ull << codeLength)) <= t);
                            auto code = ((l << needMsb) | (l >> codeLength));
                            codeLength += needMsb;
                
                                        code &= ((1ull << codeLength) - 1); // TEMP
                            result[maxLeft][maxRight][left][total] = {code, codeLength};
                        }
                    }
                }
            }
        }
        return true;
    }(tinyEncodeTable);


    //======================================================================================================================
    void pack_value
    (
        m99_encode_stream & encodeStream,
        std::uint32_t left,
        std::uint32_t total,
        std::uint32_t maxLeft,
        std::uint32_t maxRight
    )
    {
        if (total < 8)
        {
            auto const & encTableEntry = tinyEncodeTable[(maxLeft >= 8) ? 7 : maxLeft][(maxRight >= 8) ? 7 : maxRight][left][total];
            encodeStream.push(encTableEntry.value_, encTableEntry.length_);
            return;
        }

        if (total > maxLeft)
        {
            auto inferredRight = (total - maxLeft);
            maxRight -= inferredRight;
            total -= inferredRight;
        }
        if (total > maxRight)
        {
            auto inferredLeft = (total - maxRight);
            left -= inferredLeft;
            total -= inferredLeft;
        }
        if (total)
        {
            std::uint32_t codeLength = 1;
            while (total >> ++codeLength)
                ;
            --codeLength;
            auto needMsb = ((left | (1ull << codeLength)) <= total);
            auto code = ((left << needMsb) | (left >> codeLength));

            codeLength += needMsb;
                        code &= ((1ull << codeLength) - 1); // TEMP
            encodeStream.push(code, codeLength);
        }
    }


    //==========================================================================
    void merge
    (
        m99_encode_stream & encodeStream,
        std::uint8_t const * begin,
        std::uint32_t totalSize,
        std::uint32_t leftSize,
        symbol_info * result,
        std::uint32_t leadingRunLength
    )
    {
        if (leadingRunLength >= totalSize)
        {
            result[0] = {begin[0], totalSize};
            return;
        }
        if (totalSize <= 2)
        {
            if (totalSize == 2)
            {
                auto c = (unsigned)(begin[0] < begin[1]);
                result[0] = {begin[!c], 1 + (unsigned)(begin[0] == begin[1])};
                result[1] = {begin[c], 1};
                encodeStream.push(c, begin[0] != begin[1]);
            }
            else
            {
                result[0] = {begin[0], 1};
            }
            return;
        }

        std::uint32_t rightSize = (totalSize - leftSize);
        symbol_info left[256];
        symbol_info right[256];
        symbol_info const * current[2] = {left, right};
        symbol_info * resultCurrent = result;
        static auto constexpr leftSide = 0;
        static auto constexpr rightSide = 1;
        auto rightLeadingRunLength = (leadingRunLength > leftSize) ? (leadingRunLength - leftSize) : [](std::uint8_t const * begin, std::uint8_t const * end)
        {
            auto cur = begin;
            auto s = *cur;
            while ((cur < end) && (*cur == s))
                ++cur;
            return std::distance(begin, cur);
        }(begin + leftSize, begin + totalSize);

        merge(encodeStream, begin + leftSize, rightSize, rightSize >> 1, right, rightLeadingRunLength);
        merge(encodeStream, begin, leftSize, leftSize >> 1, left, leadingRunLength);

        #pragma pack(push, 1)
        using size_union = union size_union
        {
            size_union(std::uint32_t left, std::uint32_t right):size_({left, right}){};
            std::size_t union_;
            struct
            {
                std::uint32_t left_;
                std::uint32_t right_;
            } size_;
        };
        #pragma pack(pop)

        std::array<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>, 256> valuesToEncode;
        std::uint32_t numValuesToEncode{0};

        size_union partitionSize_(leftSize, rightSize);
        while (partitionSize_.size_.left_ && partitionSize_.size_.right_)
        {
            size_union count(
                (-(current[leftSide]->symbol_ <= current[rightSide]->symbol_) & (std::uint32_t)current[leftSide]->count_),
                (-(current[rightSide]->symbol_ <= current[leftSide]->symbol_) & (std::uint32_t)current[rightSide]->count_)
            );
            auto totalCount = (count.size_.left_ + count.size_.right_);
            valuesToEncode[numValuesToEncode++] = {count.size_.left_, totalCount, partitionSize_.size_.left_, partitionSize_.size_.right_};
            partitionSize_.union_ -= count.union_;
            *resultCurrent++ = {current[(count.size_.left_ == 0)]->symbol_, totalCount};
            current[leftSide] += (count.size_.left_ != 0);
            current[rightSide] += (count.size_.right_ != 0);
        }
        auto n = partitionSize_.size_.left_ + partitionSize_.size_.right_;
        symbol_info const * c = current[(partitionSize_.size_.left_ == 0)];
        while (n > 0)
        {
            n -= c->count_;
            *resultCurrent++ = *c++;
        }


        while (numValuesToEncode)
        {
            auto [left, total, maxLeft, maxRight] = valuesToEncode[--numValuesToEncode];
            pack_value(encodeStream, left, total, maxLeft, maxRight);
        }
    }

} // namespace


//==========================================================================
void maniscalco::m99_encode
(
    std::uint8_t const * begin,
    std::uint8_t const * end,
    m99_encode_stream & encodeStream

)
{
    // determine initial merge boundary (left size is largest power of 2 that is less than the input size).
    std::uint32_t bytesToEncode = std::distance(begin, end);
    std::uint32_t leftSize = 1;
    while (leftSize < bytesToEncode)
        leftSize <<= 1;
    symbol_info symbolList[256];

    // do recursive merge and encode
    auto cur = begin;
    auto s = *cur;
    while ((cur < end) && (*cur == s))
        ++cur;
    auto leadingRunLength = std::distance(begin, cur);
    merge(encodeStream, begin, bytesToEncode, leftSize >> 1, symbolList, leadingRunLength);

    // encode the symbols and their counts 
    auto n = bytesToEncode;
    std::vector<std::tuple<std::uint8_t, std::uint32_t, std::uint32_t>> headerValuesToEncode;
    headerValuesToEncode.reserve(256);
    for (auto & symbolInfo : symbolList)
    {
        if (n == 0)
            break;
        headerValuesToEncode.push_back({symbolInfo.symbol_, symbolInfo.count_, n});
        n -= symbolInfo.count_;
    }
    std::reverse(headerValuesToEncode.begin(), headerValuesToEncode.end());
    for (auto [symbol, count, maxCount] : headerValuesToEncode)
    {
        encodeStream.push(symbol, 8);
        pack_value(encodeStream, count, maxCount, maxCount, maxCount);
    }
    encodeStream.push(1, 1);
}
