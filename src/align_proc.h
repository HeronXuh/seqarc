#ifndef ALIGN_PROC_H
#define ALIGN_PROC_H

#include <iostream>
#include <vector>
#include <map>
#include <math.h>
#include <fstream>

using namespace std;

typedef struct {
    int blockNum;
    int blockPos;
    bool isRev;
    int* cigar_l;
    int* cigar_v;
} align_info;

class bitProc{
public:
    int bit4int(int input); //返回表示int所需的bit数
    string int2bit(int input, int byte4output); //将a转成byte4output位的二进制赋给str，即在前填0
    string char2bit(char input); //将压缩文件中的char转回二进制
    int bit2int(string input); //将二进制转回十进制
};

int bitProc::bit4int(int input) {
    int bitNum = 1;
    while (pow(2, bitNum) < input+1) { //consider 0
        bitNum++;
    }
    return bitNum;
}

string bitProc::int2bit(int input, int byte4output){
    if (byte4output < bit4int(input)){
        cout << input << '\t' << byte4output << endl;
        cout << "err: bytes not long enough to represent input." << endl;
        exit(-1);
    }
    string output(byte4output - bit4int(input), '0');
    char *p = (char *) &input, c = 0, f = 0;//p指向a的首地址
    for (int o = 0; o < 4; ++o) {
        for (int i = 0; i < 8; ++i) {
            c = p[3 - o] & (1 << (7 - i));
            if (!f && !(f = c))continue;
            output += c ? "1" : "0";
        }
    }
    return output;
}

string bitProc::char2bit(char input) {
    string output;
    for (int i = 0; i < CHAR_BIT; ++i) {
        output = to_string((long long int)((input >> i) & 1)) + output;
    }
    return output;
}

int bitProc::bit2int(string input) {
    int output = 0;
    for(int i=0;i<input.length(); i++) {
        output = output * 2 + (input[i] - '0');
    }
    return output;
}

/*********** encode ***********/
class encode : public bitProc{
public:
    encode(){;};
    encode(int MaxMis_, int MaxInsr_, int MaxReadLen_);
    void parse_h(int byte4pos_, fstream &out);
    void parse_1(align_info &align_info1, int readLen, fstream &out);
    void parse_2(align_info &align_info2, int readLen, fstream &out);
    void end(fstream &out);

private:
    int MaxMis, MaxInsr, MaxReadLen;
    int max_readLen_bit;
    int max_insr_bit;
    int max_mis_bit;

    bool init1, init2;
    int byte4pos, tmp_pos1, last_readLen1, last_readLen2, cigar_num;
    string buffer, cigar_l, cigar_v;
    map<int, string> cigarv2bit;

    void bufferOut(string &buffer, fstream &output); //将buffer中的内容以8bit为单位输出到output
};

encode::encode(int MaxMis_, int MaxInsr_, int MaxReadLen_){
    init1 = true;
    init2 = true;
    MaxMis = MaxMis_;
    MaxInsr= MaxInsr_;
    MaxReadLen = MaxReadLen_;
    max_readLen_bit = bit4int(MaxReadLen);
    max_insr_bit = bit4int(MaxInsr);
    max_mis_bit = bit4int(MaxMis);
    cigarv2bit[0]="0"; cigarv2bit[1]="10"; cigarv2bit[2]="11";
}

void encode::parse_h(int byte4pos_, fstream &out) {
    byte4pos = byte4pos_;
    buffer += int2bit(MaxMis, 8);
    buffer += int2bit(MaxInsr, 16);
    buffer += int2bit(MaxReadLen, 16);
    buffer += int2bit(byte4pos, 8);
    bufferOut(buffer, out);
}

void encode::parse_1(align_info &align_info1, int readLen, fstream &out) {
    if (init1){ // 第一条read
        init1 = false;
        buffer += "1"; //Block有内容
        last_readLen1 = readLen;
        buffer += int2bit(readLen, max_readLen_bit);
    }
    else{
        if (readLen == last_readLen1)
            buffer += "0";
        else{
            buffer += readLen<=last_readLen1 ? "10" : "11";
            buffer += int2bit(abs(last_readLen1-readLen), max_readLen_bit);
            last_readLen1 = readLen;
        }
    }
    tmp_pos1 = align_info1.blockPos;
    buffer += int2bit(align_info1.blockPos, byte4pos);
    buffer += align_info1.isRev;

    cigar_num = 0;
    cigar_l = cigar_v = "";
    if (align_info1.cigar_l[0] != -1){
        cigar_num ++;
        cigar_l += int2bit(align_info1.cigar_l[0], bit4int(readLen));
        cigar_v += cigarv2bit[align_info1.cigar_v[0]];
        int remainLen = readLen - align_info1.cigar_l[0]-1;
        for (int i=1;i<MaxMis;i++){
            if (align_info1.cigar_l[i] != -1){
                cigar_num ++;
                remainLen -= align_info1.cigar_l[i-1] + 1;
                cigar_l += int2bit(align_info1.cigar_l[i], bit4int(remainLen));
                cigar_v += cigarv2bit[align_info1.cigar_v[i]];
            }
            else
                break;
        }
    }
    buffer += int2bit(cigar_num, max_mis_bit);
    buffer += cigar_l;
    buffer += cigar_v;
    bufferOut(buffer, out);
}

void encode::parse_2(align_info &align_info2, int readLen, fstream &out) {
    if (init2){ // 第一条read
        init2 = false;
        last_readLen2 = readLen;
        buffer += int2bit(readLen, max_readLen_bit);
    }
    else{
        if (readLen == last_readLen2)
            buffer += "0";
        else{
            buffer += last_readLen2<readLen ? "10" : "11";
            buffer += int2bit(abs(last_readLen2-readLen), max_readLen_bit);
            last_readLen2 = readLen;
        }
    }
    if (tmp_pos1 <=align_info2.blockPos)
        buffer += "0" + int2bit(align_info2.blockPos-tmp_pos1, max_insr_bit);
    else
        buffer += "1" + int2bit(tmp_pos1-align_info2.blockPos, max_insr_bit);
    buffer += align_info2.isRev;

    cigar_num = 1;
    cigar_l = cigar_v = "";
    cigar_l += int2bit(align_info2.cigar_l[0], bit4int(readLen));
    cigar_v += cigarv2bit[align_info2.cigar_v[0]];

    for (int i=1;i<MaxMis;i++){
        if (align_info2.cigar_l[i] != -1){
            cigar_num ++;
            cigar_l += int2bit(align_info2.cigar_l[i], bit4int(readLen-align_info2.cigar_l[i-1]));
            cigar_v += cigarv2bit[align_info2.cigar_v[i]];
        }
        else
            break;
    }
    buffer += int2bit(cigar_num, max_mis_bit);
    buffer += cigar_l;
    buffer += cigar_v;

    bufferOut(buffer, out);
}

void encode::end(fstream &out) {
    if (init1){ //block内无read
        buffer += "0";
    }
    else{
        buffer += "11";
        buffer += int2bit(abs(last_readLen1), max_readLen_bit); //Len=0 for breakpoint
        bufferOut(buffer, out);
    }
    if (!buffer.length())
        buffer += string(8-buffer.length(), '0');
    bufferOut(buffer, out);
}

void encode::bufferOut(string &buffer, fstream &output)
{
    unsigned int count = 0;
    while (buffer.length() >= (count+1)*8){
        char c='\0';
        for(int i=0;i<8;i++)
        {
            if(buffer[8*count+i]=='1') c=(c<<1)|1;
            else c=c<<1;
        }
        output.write(&c, sizeof(char));
        count++;
    }
    buffer.erase(0,8*count);
}

/***********decode***********/
class decode : public bitProc{
public:
    decode();
    void parse_h(fstream &in);
    void parse_se(align_info &align_info1, fstream &in);
    void parse_pe(align_info &align_info1, fstream &in);

private:
    bool init, nextBlock;
    int MaxMis, MaxInsr, MaxReadLen, byte4pos, readLen1, readLen2;
    int max_readLen_bit, max_mis_bit;
    int tmp_pos1, cigar_num, tmp;
    int peMark;

    string buffer, cigar_l, cigar_v;
    string bufferIn(int length, fstream &in); //length指的是byte数，返回的是二进制字符串
};

decode::decode()
{
    init = true;
    peMark = 1;
    readLen2 = 0;
}

void decode::parse_h(fstream &in) {
    in.read((char *)&MaxMis, 1);
    in.read((char *)&MaxInsr, 2);
    in.read((char *)&MaxReadLen, 2);
    in.read((char *)&byte4pos, 1);
    max_mis_bit = bit4int(MaxMis);
    max_readLen_bit = bit4int(MaxReadLen);
}

void decode::parse_se(align_info &align_info1, fstream &in){
    if (init){
        init = false;
        in.read((char *)&MaxMis, 1);
        in.read((char *)&MaxInsr, 2);
        in.read((char *)&MaxReadLen, 2);
        in.read((char *)&byte4pos, 1);
        max_mis_bit = bit4int(MaxMis);
        max_readLen_bit = bit4int(MaxReadLen);
        readLen1 = bit2int(bufferIn(max_readLen_bit,in));
    }
    else{
        if (bufferIn(1,in) != "0"){ //readLen发生变化
            if (bufferIn(1,in) != "0")
                readLen1 += bit2int(bufferIn(max_readLen_bit,in));
            else
                readLen1 -= bit2int(bufferIn(max_readLen_bit,in));
        }
    }
    align_info1.blockPos = bit2int(bufferIn(byte4pos,in));
    align_info1.isRev = (bool)atoi(bufferIn(1,in).c_str());
    cigar_num = bit2int(bufferIn(max_mis_bit, in));
    tmp = 0;
    for (int i=0; i< cigar_num; i++){
        align_info1.cigar_l[i] = bit2int(bufferIn(bit4int(readLen1-tmp),in));
        align_info1.cigar_v[i] = bit2int(bufferIn(2,in));
        tmp += align_info1.cigar_l[i];
    }
}

void decode::parse_pe(align_info &align_info1, fstream &in){
    if (init){
        init = false;
        readLen1 = bit2int(bufferIn(max_readLen_bit,in));
    }
    else{
        if (!peMark){ //PE1
            if (bufferIn(1,in) != "0"){ //readLen发生变化
                if (bufferIn(1,in) != "0")
                    readLen2 += bit2int(bufferIn(max_readLen_bit,in));
                else
                    readLen2 -= bit2int(bufferIn(max_readLen_bit,in));
            }
            peMark++;
        }
        else{ //PE2
            if (!readLen2) //init
                readLen2 = bit2int(bufferIn(max_readLen_bit,in));
            else{
                if (bufferIn(1,in) != "0"){ //readLen发生变化
                    if (bufferIn(1,in) != "0")
                        readLen2 += bit2int(bufferIn(max_readLen_bit,in));
                    else
                        readLen2 -= bit2int(bufferIn(max_readLen_bit,in));
                }
            }
            peMark--;
        }
        align_info1.blockPos = bit2int(bufferIn(byte4pos,in));
        align_info1.isRev = (bool)atoi(bufferIn(1,in).c_str());
        cigar_num = bit2int(bufferIn(max_mis_bit, in));
        tmp = 0;
        for (int i=0; i< cigar_num; i++){
            align_info1.cigar_l[i] = bit2int(bufferIn(bit4int(readLen1-tmp),in));
            align_info1.cigar_v[i] = bit2int(bufferIn(2,in));
            tmp += align_info1.cigar_l[i];
        }
    }
}

string decode::bufferIn(int length, fstream &in) {
    if (length < buffer.length()){ //buffer的内容不够，需要向in读取
        int bitNum = (int)floor((buffer.length() - length)/8);
        char tmpIn;
        for (int i=0; i< bitNum; i++){
            in.read(&tmpIn, 1);
            buffer += char2bit(tmpIn);
        }
    }
    string output;
    output = buffer.substr(0, length);
    buffer.erase(0, length);
    return output;
}

#endif //ALIGN_PROC_H
