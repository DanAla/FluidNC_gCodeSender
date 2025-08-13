/**
 * core/GCodeParser.h
 * Comprehensive G-code parser with proper state machine and modal command handling
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>

// Forward declarations
struct GCodeCommand;
struct GCodeState;
struct ParsedLine;
struct ToolpathSegment;

// G-code command types
enum class CommandType {
    RAPID_MOVE,         // G0
    LINEAR_MOVE,        // G1
    CW_ARC,            // G2
    CCW_ARC,           // G3
    DWELL,             // G4
    PLANE_XY,          // G17
    PLANE_XZ,          // G18
    PLANE_YZ,          // G19
    INCHES,            // G20
    MILLIMETERS,       // G21
    RETURN_HOME,       // G28
    RETURN_PREDEFINED, // G30
    COORDINATE_OFFSET, // G92
    ABSOLUTE_MODE,     // G90
    INCREMENTAL_MODE,  // G91
    FEED_RATE_MODE,    // G93/G94/G95
    SPINDLE_CW,        // M3
    SPINDLE_CCW,       // M4
    SPINDLE_STOP,      // M5
    TOOL_CHANGE,       // M6
    COOLANT_MIST,      // M7
    COOLANT_FLOOD,     // M8
    COOLANT_OFF,       // M9
    PROGRAM_END,       // M2/M30
    PROGRAM_STOP,      // M0
    OPTIONAL_STOP,     // M1
    WORK_COORD_1,      // G54
    WORK_COORD_2,      // G55
    WORK_COORD_3,      // G56
    WORK_COORD_4,      // G57
    WORK_COORD_5,      // G58
    WORK_COORD_6,      // G59
    CANNED_CYCLE_DRILL,// G81
    CANNED_CYCLE_DWELL,// G82
    CANNED_CYCLE_PECK, // G83
    CANNED_CYCLE_TAP,  // G84
    CANNED_CYCLE_BORE, // G85
    CANCEL_CYCLE,      // G80
    UNKNOWN
};

// Units of measurement
enum class Units {
    MILLIMETERS,
    INCHES
};

// Coordinate systems
enum class CoordinateSystem {
    G54, G55, G56, G57, G58, G59
};

// Plane selection
enum class Plane {
    XY,  // G17
    XZ,  // G18
    YZ   // G19
};

// Motion mode
enum class MotionMode {
    ABSOLUTE_MODE,     // G90
    INCREMENTAL_MODE   // G91
};

// Feed rate mode
enum class FeedRateMode {
    UNITS_PER_MINUTE,    // G94
    INVERSE_TIME,        // G93
    UNITS_PER_REV        // G95
};

// Spindle state
enum class SpindleState {
    OFF,
    CW,
    CCW
};

// Coolant state
struct CoolantState {
    bool mist = false;   // M7
    bool flood = false;  // M8
};

// Position structure
struct Position {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    
    bool hasX = false;
    bool hasY = false;
    bool hasZ = false;
    bool hasA = false;
    bool hasB = false;
    bool hasC = false;
    
    void clear() {
        hasX = hasY = hasZ = hasA = hasB = hasC = false;
    }
};

// Arc parameters
struct ArcParameters {
    double i = 0.0;
    double j = 0.0;
    double k = 0.0;
    double r = 0.0;  // Radius format
    
    bool hasI = false;
    bool hasJ = false;
    bool hasK = false;
    bool hasR = false;
    
    void clear() {
        hasI = hasJ = hasK = hasR = false;
    }
};

// Complete G-code state machine
struct GCodeState {
    // Current position (absolute coordinates)
    Position currentPosition;
    Position workOffset;     // Current work coordinate offset
    
    // Modal states that persist between lines
    CommandType motionMode = CommandType::RAPID_MOVE;
    Units units = Units::MILLIMETERS;
    CoordinateSystem coordinateSystem = CoordinateSystem::G54;
    Plane plane = Plane::XY;
    MotionMode positionMode = MotionMode::ABSOLUTE_MODE;
    FeedRateMode feedRateMode = FeedRateMode::UNITS_PER_MINUTE;
    
    // Machine states
    SpindleState spindleState = SpindleState::OFF;
    CoolantState coolantState;
    int currentTool = 0;
    
    // Modal values
    double feedRate = 0.0;      // F value
    double spindleSpeed = 0.0;  // S value
    double dwellTime = 0.0;     // P value for G4
    
    // Canned cycle parameters
    double retractHeight = 0.0; // R value
    double cycleDepth = 0.0;    // Z value in canned cycles
    double peckIncrement = 0.0; // Q value for G83
    
    // Program flow
    bool programRunning = true;
    int lineNumber = 0;
    
    // Reset to initial state
    void reset();
    
    // Apply work coordinate system offset
    Position getAbsolutePosition(const Position& pos) const;
};

// Parsed G-code command structure
struct GCodeCommand {
    CommandType type = CommandType::UNKNOWN;
    Position position;
    ArcParameters arc;
    
    // Parameters
    double feedRate = -1;     // F value (-1 = not specified)
    double spindleSpeed = -1; // S value (-1 = not specified)
    double dwellTime = -1;    // P value (-1 = not specified)
    double retractHeight = -1;// R value (-1 = not specified)
    double peckIncrement = -1;// Q value (-1 = not specified)
    int toolNumber = -1;      // T value (-1 = not specified)
    
    int lineNumber = 0;
    std::string originalLine;
    std::string comment;
};

// Result of parsing a single line
struct ParsedLine {
    std::vector<GCodeCommand> commands;
    std::string comment;
    std::string originalLine;
    int lineNumber = 0;
    bool hasError = false;
    std::string errorMessage;
};

// Toolpath segment for visualization
struct ToolpathSegment {
    enum Type {
        RAPID,
        LINEAR,
        ARC_CW,
        ARC_CCW,
        DRILL_CYCLE
    };
    
    Type type;
    Position start;
    Position end;
    Position center;    // For arcs
    double radius = 0.0; // For arcs
    double feedRate = 0.0;
    double spindleSpeed = 0.0;
    bool spindleOn = false;
    bool coolantOn = false;
    int toolNumber = 0;
    
    // Calculated values
    double length = 0.0;        // Segment length
    double estimatedTime = 0.0; // Time estimate in seconds
};

// Statistics from parsing
struct GCodeStatistics {
    int totalLines = 0;
    int commandLines = 0;
    int commentLines = 0;
    int errorLines = 0;
    
    int rapidMoves = 0;
    int linearMoves = 0;
    int arcMoves = 0;
    int toolChanges = 0;
    
    double totalDistance = 0.0;         // Total toolpath distance
    double rapidDistance = 0.0;         // Rapid move distance
    double cuttingDistance = 0.0;       // Cutting move distance
    double estimatedTime = 0.0;         // Total estimated time (minutes)
    
    Position minBounds, maxBounds;      // Bounding box
    bool boundsValid = false;
    
    std::set<int> toolsUsed;            // Set of tools used
    std::map<double, double> feedRates; // Feed rate usage (rate -> distance)
    std::map<double, double> spindleSpeeds; // Spindle speed usage (rpm -> time)
    
    void reset();
};

// Error information
struct ParseError {
    int lineNumber;
    std::string line;
    std::string message;
    enum Severity { WARNING, PARSE_ERROR, FATAL } severity;
};

// Parser callbacks for real-time updates
using ProgressCallback = std::function<void(int currentLine, int totalLines)>;
using ErrorCallback = std::function<void(const ParseError& error)>;
using SegmentCallback = std::function<void(const ToolpathSegment& segment)>;

// Main G-code parser class
class GCodeParser {
public:
    GCodeParser();
    ~GCodeParser();
    
    // Main parsing methods
    bool parseFile(const std::string& filename);
    bool parseString(const std::string& gcode);
    ParsedLine parseLine(const std::string& line, int lineNumber = 0);
    
    // State management
    void resetState();
    const GCodeState& getState() const { return m_state; }
    
    // Results
    const std::vector<ToolpathSegment>& getToolpath() const { return m_toolpath; }
    const GCodeStatistics& getStatistics() const { return m_statistics; }
    const std::vector<ParseError>& getErrors() const { return m_errors; }
    
    // Configuration
    void setStrictMode(bool strict) { m_strictMode = strict; }
    void setMaxErrorCount(int maxErrors) { m_maxErrors = maxErrors; }
    void enableStatistics(bool enable) { m_calculateStatistics = enable; }
    void enableToolpathGeneration(bool enable) { m_generateToolpath = enable; }
    
    // Callbacks
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    void setErrorCallback(ErrorCallback callback) { m_errorCallback = callback; }
    void setSegmentCallback(SegmentCallback callback) { m_segmentCallback = callback; }
    
    // Utility methods
    static std::string commandTypeToString(CommandType type);
    static bool isModalCommand(CommandType type);
    static bool isMotionCommand(CommandType type);
    
    // Coordinate transformation
    Position transformToWorkCoordinates(const Position& machinePos) const;
    Position transformToMachineCoordinates(const Position& workPos) const;
    
private:
    // Internal parsing methods
    bool parseGCode(int gcode, GCodeCommand& command);
    bool parseMCode(int mcode, GCodeCommand& command);
    bool parseParameters(const std::string& line, GCodeCommand& command);
    std::string extractComment(const std::string& line);
    std::string cleanLine(const std::string& line);
    
    // Command processing
    void processCommand(const GCodeCommand& command);
    void updateModalState(const GCodeCommand& command);
    void generateToolpathSegment(const GCodeCommand& command);
    void generateToolpathSegmentFromPositions(const GCodeCommand& command, const Position& startPos, const Position& endPos);
    void calculateArcCenter(const GCodeCommand& command, Position& center, double& radius);
    void calculateArcCenterFromPositions(const GCodeCommand& command, const Position& startPos, const Position& endPos, Position& center, double& radius);
    void expandCannedCycle(const GCodeCommand& command);
    
    // Statistics and validation
    void updateStatistics(const GCodeCommand& command);
    void updateBounds(const Position& pos);
    bool validateCommand(const GCodeCommand& command, std::string& error);
    void reportError(const std::string& message, int lineNumber, 
                     ParseError::Severity severity = ParseError::PARSE_ERROR);
    
    // State variables
    GCodeState m_state;
    std::vector<ToolpathSegment> m_toolpath;
    GCodeStatistics m_statistics;
    std::vector<ParseError> m_errors;
    
    // Configuration
    bool m_strictMode = false;
    bool m_calculateStatistics = true;
    bool m_generateToolpath = true;
    int m_maxErrors = 100;
    
    // Callbacks
    ProgressCallback m_progressCallback;
    ErrorCallback m_errorCallback;
    SegmentCallback m_segmentCallback;
    
    // Internal lookup tables
    static std::map<int, CommandType> s_gcodeLookup;
    static std::map<int, CommandType> s_mcodeLookup;
    static void initializeLookupTables();
    static bool s_tablesInitialized;
};
