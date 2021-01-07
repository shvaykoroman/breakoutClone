
// TODO(shvayko): should it be in global scope?
global u32 randNum = 123456;

// TODO(shvayko): bad working random system! reimplement
inline u32
getRandomNumber()
{
    u32 result = randNum;
    
    result ^= result << 13;
    result ^= result >> 17;
    result ^= result << 5;
    randNum = result;
    return result;
}

inline bool
randomChoice(s32 chance)
{
    bool result = false;
    
    result = (getRandomNumber() % chance) == 0;
    
    return result;
}

inline u32
getRandomNumberInRange(u32 min, u32 max)
{
    u32 result = 0;
    u32 range = max - min;
    
    result = (getRandomNumber() % range) + min;
    
    return result;
}
