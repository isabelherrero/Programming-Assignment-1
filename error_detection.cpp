// Used for input/output operations
#include <iostream>

// For file input to read from data.txt
#include <fstream>

// Used for binary manipulation
#include <bitset>

// Enable string class
#include <string>

// Enable arrays
#include <vector>

// Allows to not have to write std in everything, cleaner code
// Learned this at HP Inc. previous employment
using namespace std;

// CRC-16 generator: x^16 + x^10 + x^4 + 1 in binary form
const string crc16Generator = "10000010000010001";

// XOR operation to the codeword to introduce the random bits error
string xorOperation(const string &firstString, const string &secondString) {
    string xorResult = "";
    for (size_t i = 0; i < secondString.length(); ++i)
        xorResult += (firstString[i] == secondString[i]) ? '0' : '1';
    return xorResult;
}

// Append n zeros to the end of a binary string
string appendZeros(const string &binaryData, int n) {
    return binaryData + string(n, '0');
}

// Calculate CRC remainder using polynomial division
string calculateCRC(const string &binaryData, const string &crc16Generator) {
    int polynomialLength = crc16Generator.length();
    string paddedBinaryData = appendZeros(binaryData, polynomialLength - 1);
    string crcRemainder = paddedBinaryData.substr(0, polynomialLength);
    
    for (size_t i = polynomialLength; i <= paddedBinaryData.length(); ++i) {
        if (crcRemainder[0] == '1')
            crcRemainder = xorOperation(crcRemainder, crc16Generator) + (i < paddedBinaryData.length() ? paddedBinaryData[i] : '0');
        else
            crcRemainder = xorOperation(crcRemainder, string(polynomialLength, '0')) + (i < paddedBinaryData.length() ? paddedBinaryData[i] : '0');
    }
    return crcRemainder.substr(1);
}

//Introduce error using XOR operation
string introduceError(const string &binaryCodeword, const string &errorBit) {
    string paddedErrorBit = errorBit;
    
    // Check if padding is needed
    if (paddedErrorBit.length() < binaryCodeword.length()) {
        int zerosToAppend = binaryCodeword.length() - paddedErrorBit.length();
        paddedErrorBit = appendZeros(paddedErrorBit, zerosToAppend);
    }
    
    // Perform the XOR operation on the equal length strings
    string corruptedCodeword = xorOperation(binaryCodeword, paddedErrorBit);
    
    return corruptedCodeword;
}

// Split string into 16-bit data blocks
vector<string> splitBlocks(const string &binaryData, int blockSize) {
    vector<string> dataBlocks;
    for (size_t i = 0; i < binaryData.size(); i += blockSize) {
        string binaryDataSegment = binaryData.substr(i, blockSize);
        if (binaryDataSegment.length() < blockSize) {
            int zerosToAppend = blockSize - binaryDataSegment.length();
            binaryDataSegment = appendZeros(binaryDataSegment, zerosToAppend);
        }
        dataBlocks.push_back(binaryDataSegment);
    }
    return dataBlocks;
}

// Helper function that performs the checksum calculation
unsigned int calculateChecksumValue(const string &binaryData) {
    vector<string> dataBlocks = splitBlocks(binaryData, 16);
    unsigned int blockSum = 0;
    for (const string &binaryDataSegment : dataBlocks)
        blockSum += stoi(binaryDataSegment, nullptr, 2);

    while (blockSum >> 16)
        blockSum = (blockSum & 0xFFFF) + (blockSum >> 16);
        
    return ~blockSum & 0xFFFF;
}

// Generates a 16-bit checksum 
string calculateTotalChecksum(const string &binaryData) {
    unsigned int finalChecksum = calculateChecksumValue(binaryData);
    bitset<16> b_finalChecksum(finalChecksum);
    return b_finalChecksum.to_string();
}

// Recalculates the checksum and checks if it validates
bool verifyChecksum(const string &receivedData) {
    return (calculateChecksumValue(receivedData) == 0);
}

// Processing function to run CRC and Checksum
void errorDetection(const string &binaryData, const string &errorBit) {
    cout << "Data:                 " << binaryData << endl;
    cout << "Error:                " << errorBit << endl;

    // CRC
    string crcRemainder = calculateCRC(binaryData, crc16Generator);
    string crcCodeword = binaryData + crcRemainder;
    cout << "CRC remainder:        " << crcRemainder << endl;
    string corruptedCRC = introduceError(crcCodeword, errorBit);
    string receivedCRCRemainder = calculateCRC(corruptedCRC, crc16Generator);
    cout << "CRC result:           " << ((stoi(receivedCRCRemainder, nullptr, 2) == 0) ? "Pass" : "Not pass") << endl;

    // Checksum
    string checksumValue = calculateTotalChecksum(binaryData);
    string checksumCodeword = binaryData + checksumValue;
    cout << "Checksum:             " << checksumValue << endl;
    string corruptedChecksum = introduceError(checksumCodeword, errorBit);
    bool checksumResult = verifyChecksum(corruptedChecksum);
    cout << "Checksum result:      " << (checksumResult ? "Pass" : "Not pass") << endl;

    cout << "======================================================================================================" << endl;
}

int main() {
    // Open data file, got this from Discussion 2 presentation
    ifstream fin("data.txt");
    
    // Data file has two items to be read format string
    string binaryData; 
    string errorBit;

    // It reads two pieces of data from the input stream fin and stores them in the data and error variables
    // If the condition is true inside the parenthesis, then it executed process_line()
    // If the condition is false, the loop is terminated
    while (fin >> binaryData >> errorBit) {

        // Function to process the data in the file
        errorDetection(binaryData, errorBit);
        }

    // Close data file because it utilizing its contents
    fin.close();
    return 0;
}



