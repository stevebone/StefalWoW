
namespace FSBGossipUtils
{
    // Converts an int64 price in copper to a string like 10 silver
    std::string MoneyToString(int64 price);

    // Builds the hire option text dynamically
    std::string BuildHireText(int64 price, uint32 hours);
}
