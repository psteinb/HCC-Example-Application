/**********************************************************************
Copyright �2013 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

�   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
�   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#include "ArrayBandwidth.hpp"

/******************************************************************************
* Implementation of ArrayBandwidth::setZero()                            *
******************************************************************************/


template <class T>
int ArrayBandwidth<T>::setZero(std::vector<T>& arry,const int width,const int height)
{
    for(int i = 0; i<height ; i++)
    {
        for(int j =0; j<width; j++)
        {
            arry.push_back(T(0));
        }
    }
    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of ArrayBandwidth::setup()                              *
******************************************************************************/
template <class T>
int ArrayBandwidth<T>::setup()
{
    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);

    //setup random data
    fillRandom(input,inputLength,1,0,T(inputLength-1),0);

    setZero(outputReadSingle,outputLength,1);
    setZero(outputReadLinear,outputLength,1);
    setZero(outputReadLU,outputLength,1);
    setZero(outputReadRandom,outputLength,1);
    setZero(outputReadStride,outputLength,1);
    setZero(outputWriteLinear,inputLength,1);
    sampleTimer->stopTimer(timer);

    setupTime = (double)sampleTimer->readTimer(timer);


    return SDK_SUCCESS;

}

/******************************************************************************
* Implementation of ArrayBandwidth::run()                                *
******************************************************************************/
template <class T>
int ArrayBandwidth<T>::run()
{
    if(sampleArgs->verify)
    {
        iter = 1;
    }
    bytes = (double)(NUM_READS * sizeof(T) * iter);
    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);

    if(RunArrayBandwidthTesting() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    sampleTimer->stopTimer(timer);
    totalkernelTime = (double)(sampleTimer->readTimer(timer));
    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::RunReadSingleBandwidth()             *
******************************************************************************/
template <class T>
void ArrayBandwidth<T>::readSingleBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    parallel_for_each(result.get_extent(),[&](index<1> gid) [[hc]]
    {
        int ind = 0;
        T sum = T(0);
        for(int j =0; j< NUM_READS; j++)
        {
            sum = sum + in[ind + j];
        }
        result[gid] = sum;

    });
}

template <class T>
int ArrayBandwidth<T>::RunReadSingleBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    hc::accelerator_view accView = hc::accelerator().get_default_view();

    readSingleBandwidth(in, result);
    accView.flush();
    accView.wait();

    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);
    for(unsigned int i = 0; i < iter; i++)
    {
        readSingleBandwidth(in, result);
        accView.flush();
    }
    accView.wait();
    sampleTimer->stopTimer(timer);
    double sec = (double)(sampleTimer->readTimer(timer));//m
    // Cumulate time for each iteration
    double perf = (bytes / sec) *1e-9;
    perf = perf * outputLength;

    std::cout << ": " << perf << " GB/s" <<std::endl;

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::RunReadLinearBandwidth()             *
******************************************************************************/

template <class T>
void ArrayBandwidth<T>::readLinearBandwidth(array<T,1>& in,array<T,1>& result)
{
    parallel_for_each(result.get_extent(),[&](index<1> gid) [[hc]]
    {
        T sum = T(0);
        for(int j = 0; j < NUM_READS; j++)
        {
            sum = sum + in[gid + j];
        }
        result[gid] = sum;
    });
}

template <class T>
int ArrayBandwidth<T>::RunReadLinearBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    hc::accelerator_view accView = hc::accelerator().get_default_view();
    readLinearBandwidth(in, result);
    accView.flush();
    accView.wait();

    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);
    for(unsigned int i = 0; i < iter; i++)
    {
        readLinearBandwidth(in, result);
        accView.flush();
    }
    accView.wait();
    sampleTimer->stopTimer(timer);
    double sec = (double)(sampleTimer->readTimer(timer));//m
    // Cumulate time for each iteration
    double perf = (bytes / sec) *1e-9;
    perf = perf * outputLength;

    std::cout << ": " << perf << " GB/s" <<std::endl;

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::RunReadLUBandwidth()                 *
******************************************************************************/
template <class T>
void ArrayBandwidth<T>::readLUDBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    parallel_for_each(result.get_extent(),[&](index<1> gid) [[hc]]
    {
        T sum = T(0);
        for(int j = 0; j < NUM_READS; j++)
        {
            sum = sum + in[gid + j * OFFSET];
        }
        result[gid] = sum;
    });
}
template <class T>
int ArrayBandwidth<T>::RunReadLUBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    hc::accelerator_view accView = hc::accelerator().get_default_view();

    readLUDBandwidth(in, result);
    accView.flush();
    accView.wait();

    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);
    for(unsigned int i = 0; i < iter; i++)
    {
        readLUDBandwidth(in, result);
        accView.flush();
    }
    accView.wait();
    sampleTimer->stopTimer(timer);
    double sec = (double)(sampleTimer->readTimer(timer));//m
    // Cumulate time for each iteration
    double perf = (bytes / sec) *1e-9;
    perf = perf * outputLength;

    std::cout << ": " << perf << " GB/s" <<std::endl;

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::RunReadRandomBandwidth()             *
******************************************************************************/
template <class T>
void ArrayBandwidth<T>::readRandomBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    parallel_for_each(result.get_extent(),[&](index<1> gid) [[hc]]
    {
        T sum = T(0);
        int ind = gid[0];
        for(int j = 0; j < NUM_READS; j++)
        {
            T midval = in[ind];
            sum = sum + midval;
            ind = (int)(midval);
        }
        result[gid] = sum;
    });
}

template <class T>
int ArrayBandwidth<T>::RunReadRandomBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    hc::accelerator_view accView = hc::accelerator().get_default_view();

    readRandomBandwidth(in, result);
    accView.flush();
    accView.wait();

    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);
    for(unsigned int i = 0; i < iter; i++)
    {
        readRandomBandwidth(in, result);
        accView.flush();
    }
    accView.wait();
    sampleTimer->stopTimer(timer);
    double sec = (double)(sampleTimer->readTimer(timer));//m
    // Cumulate time for each iteration
    double perf = (bytes / sec) *1e-9;
    perf = perf * outputLength;

    std::cout << ": " << perf << " GB/s" <<std::endl;

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::RunReadStrideBandwidth()          *
******************************************************************************/
template <class T>
void ArrayBandwidth<T>::readStrideBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    parallel_for_each(result.get_extent(),[&](index<1> gid) [[hc]]
    {
        T sum = T(0);
        for(int j = 0; j < NUM_READS; j++)
        {
            sum = sum + in[gid * NUM_READS + j];
        }
        result[gid] = sum;
    });
}
template <class T>
int ArrayBandwidth<T>::RunReadStrideBandwidth(hc::array<T,1>& in,hc::array<T,1>& result)
{
    hc::accelerator_view accView = hc::accelerator().get_default_view();

    readStrideBandwidth(in, result);
    accView.flush();
    accView.wait();

    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);
    for(unsigned int i = 0; i < iter; i++)
    {
        readStrideBandwidth(in, result);
        accView.flush();
    }
    accView.wait();
    sampleTimer->stopTimer(timer);
    double sec = (double)(sampleTimer->readTimer(timer));//m
    // Cumulate time for each iteration
    double perf = (bytes / sec) *1e-9;
    perf = perf * outputLength;

    std::cout << ": " << perf << " GB/s" <<std::endl;

    return SDK_SUCCESS;

}

/******************************************************************************
* Implementation of ArrayBandwidth::RunWriteLinearBandwidth()            *
******************************************************************************/
template <class T>
void ArrayBandwidth<T>::writeLinearBandwidth(hc::array<T,1>& constValue,
        hc::array<T,1>& result)
{
    hc::extent<1> ext(outputLength);
    parallel_for_each(ext,[&](index<1> gid) [[hc]]
    {
        for(int j = 0; j < NUM_READS; j++)
        {
            result[gid + j * STRIDE] = constValue[0];
        }
    });
}
template <class T>
int ArrayBandwidth<T>::RunWriteLinearBandwidth(array<T,1>& constValue,
        array<T,1>& result)
{
    hc::accelerator_view accView = hc::accelerator().get_default_view();

    writeLinearBandwidth(constValue, result);
    accView.flush();
    accView.wait();

    int timer = sampleTimer->createTimer();
    sampleTimer->resetTimer(timer);
    sampleTimer->startTimer(timer);
    for(unsigned int i = 0; i < iter; i++)
    {
        writeLinearBandwidth(constValue, result);
        accView.flush();
    }
    accView.wait();
    sampleTimer->stopTimer(timer);
    double sec = (double)(sampleTimer->readTimer(timer));//m
    // Cumulate time for each iteration
    double perf = (bytes / sec) *1e-9;
    perf = perf * outputLength;

    std::cout << ": " << perf << " GB/s" <<std::endl;

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::RunArrayBandwidthTesting()           *
******************************************************************************/
template <class T>
int ArrayBandwidth<T>::RunArrayBandwidthTesting()
{
    std::cout<<"\n\n***********************************************"<<std::endl;
    //array declearation
    hc::array<T,1> inputArray(inputLength,input.begin());
    hc::array<T,1> outReadSingleArray(outputLength);
    hc::array<T,1> outReadLinearArray(outputLength);
    hc::array<T,1> outReadLUArray(outputLength);
    hc::array<T,1> outReadRandomArray(outputLength);
    hc::array<T,1> outReadStrideArray(outputLength);


    //clear all output vectors
    fill(outputReadSingle.begin(),outputReadSingle.end(),T(0));
    fill(outputReadLinear.begin(),outputReadLinear.end(),T(0));
    fill(outputReadLU.begin(),outputReadLU.end(),T(0));
    fill(outputReadRandom.begin(),outputReadRandom.end(),T(0));
    fill(outputReadStride.begin(),outputReadStride.end(),T(0));
    fill(outputWriteLinear.begin(),outputWriteLinear.end(),T(0));

    std::cout << "\nAccelerator Memory Read\nAccessType\t: single\n"<<std::endl;
    std::cout << "Bandwidth\t";

    RunReadSingleBandwidth(inputArray,outReadSingleArray);
    copy(outReadSingleArray,outputReadSingle.begin());

    std::cout << "\nAccelerator Memory Read\nAccessType\t: linear\n"<<std::endl;
    std::cout << "Bandwidth\t";

    RunReadLinearBandwidth(inputArray,outReadLinearArray);
    copy(outReadLinearArray,outputReadLinear.begin());

    std::cout << "\nAccelerator Memory Read\nAccessType\t: linear(uncached)\n"<<std::endl;
    std::cout << "Bandwidth\t";

    RunReadLUBandwidth(inputArray,outReadLUArray);
    copy(outReadLUArray,outputReadLU.begin());

    std::cout << "\nAccelerator Memory Read\nAccessType\t: random\n"<<std::endl;
    std::cout << "Bandwidth\t";

    RunReadRandomBandwidth(inputArray,outReadRandomArray);
    copy(outReadRandomArray,outputReadRandom.begin());

    std::cout << "\nAccelerator Memory Read\nAccessType\t: stride\n"<<std::endl;
    std::cout << "Bandwidth\t";

    RunReadStrideBandwidth(inputArray,outReadStrideArray);
    copy(outReadStrideArray,outputReadStride.begin());

    std::cout << "\nAccelerator Memory Write\nAccessType\t: linear\n"<<std::endl;
    std::cout << "Bandwidth\t";
    //for writeLinear function

    std::vector<T> constValue(1,T(10));
    array<T,1> constvalueArray(1,constValue.begin());
    array<T,1> outWriteLinearArray(inputLength);

    RunWriteLinearBandwidth(constvalueArray,outWriteLinearArray);
    copy(outWriteLinearArray,outputWriteLinear.begin());

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::verifyResults()                      *
******************************************************************************/
template <class T>
int ArrayBandwidth<T>::verifyResults()
{
    if(sampleArgs->verify)
    {
        std::vector<T> verificationOutput(outputLength,T(0));
        std::cout<<"\nVerifying results for Read-Single : ";
        //fill(verificationOutput.begin(),verificationOutput.end(),T(0));
        int index = 0;
        for(int i = 0; i<(int)outputLength; i++)
        {
            for(int j = 0; j<NUM_READS; j++)
            {
                verificationOutput[i] += input[index + j];
            }
        }
        if(verificationOutput == outputReadSingle)
        {
            std::cout<<"Passed!\n"<<std::endl;
        }
        else
        {
            std::cout<<"Failed!\n"<<std::endl;
            return SDK_FAILURE;
        }

        std::cout<<"\nVerifying results for Read-Linear : ";
        fill(verificationOutput.begin(),verificationOutput.end(),T(0));
        for(int i = 0; i < (int)outputLength; i++)
        {
            index = i;
            for(int j = 0; j < NUM_READS; j++)
            {
                verificationOutput[i] += input[index + j];
            }
        }
        if(verificationOutput == outputReadLinear)
        {
            std::cout<<"Passed!\n"<<std::endl;
        }
        else
        {
            std::cout<<"Failed!\n"<<std::endl;
            return SDK_FAILURE;
        }

        std::cout<<"\nVerifying results for Read-Linear(uncached) : ";
        fill(verificationOutput.begin(),verificationOutput.end(),T(0));
        for(int i = 0; i < (int)outputLength; i++)
        {
            index = i;
            for(int j = 0; j < NUM_READS; j++)
            {
                verificationOutput[i] += input[index + j * OFFSET];
            }
        }
        if(verificationOutput == outputReadLU)
        {
            std::cout<<"Passed!\n"<<std::endl;
        }
        else
        {
            std::cout<<"Failed!\n"<<std::endl;
            return SDK_FAILURE;
        }

        std::cout<<"\nVerifying results for Read-Random : ";
        fill(verificationOutput.begin(),verificationOutput.end(),T(0));
        for(int i = 0; i < (int)outputLength; i++)
        {
            index = i;
            for(int j = 0; j < NUM_READS; j++)
            {
                T midVal = input[index];
                verificationOutput[i] += midVal;
                index = (int)midVal;
            }
        }
        if(verificationOutput == outputReadRandom)
        {
            std::cout<<"Passed!\n"<<std::endl;
        }
        else
        {
            std::cout<<"Failed!\n"<<std::endl;
            return SDK_FAILURE;
        }

        std::cout<<"\nVerifying results for Read-Stride : ";
        fill(verificationOutput.begin(),verificationOutput.end(),T(0));
        for(int i = 0; i < (int)outputLength; i++)
        {
            index = i;
            for(int j = 0; j < NUM_READS; j++)
            {

                verificationOutput[i] += input[index * NUM_READS + j];

            }
        }
        if(verificationOutput == outputReadStride)
        {
            std::cout<<"Passed!\n"<<std::endl;
        }
        else
        {
            std::cout<<"Failed!\n"<<std::endl;
            return SDK_FAILURE;
        }

        std::cout<<"\nVerifying results for Write-Linear : ";
        fill(verificationOutput.begin(),verificationOutput.end(),T(10));
        for( int i = 0; i < (int)outputLength; i++)
        {
            if(outputWriteLinear[i] == verificationOutput[i])
            {
                if(i == outputLength - 1)
                {
                    std::cout<<"Passes!\n"<<std::endl;
                }

            }
            else
            {
                std::cout<<"Failed!\n"<<std::endl;
                return SDK_FAILURE;
            }

        }

    }
    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::initialize()                         *
******************************************************************************/
template <class T>
int ArrayBandwidth<T>::initialize()
{
    //Call base class Initialize to get default configuration
    if(sampleArgs->initialize())
    {
        return SDK_FAILURE;
    }

    Option* num_iterations = new Option;
    CHECK_ALLOCATION(num_iterations,"num_iterators memory allocation failed");

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations for kernel execution";
    num_iterations->_type = CA_ARG_INT;
    num_iterations->_value = &iter;

    sampleArgs->AddOption(num_iterations);
    delete num_iterations;

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of ArrayBandwidth::fillRandom()                                *
******************************************************************************/
template <class T>
int ArrayBandwidth<T>::fillRandom(
    std::vector<T>& arrayPtr,
    const int width,
    const int height,
    const T rangeMin,
    const T rangeMax,
    unsigned int seed)
{
    if(!seed)
    {
        seed = (unsigned int)time(NULL);
    }
    srand(seed);
    double range = double(rangeMax - rangeMin) + 1.0;
    /* random initialisation of input */
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            int index = i*width + j;
            if((int)(arrayPtr.size()) >=width * height)
            {
                arrayPtr[index] = rangeMin + T(range*rand()/(RAND_MAX + 1.0));
            }
            else
            {
                arrayPtr.push_back(rangeMin + T(range*rand()/(RAND_MAX + 1.0)));
            }
        }
    }

    return SDK_SUCCESS;
}

/*************************************************************************************
**************************************************************************************/


int main(int argc, char* argv[])
{
    std::cout << "************************************************" << std::endl;
    std::cout << "              ArrayBandwidth " << std::endl;
    std::cout << "************************************************" << std::endl;
    std::cout << std::endl;

    int status = 0;

    /*******************************************************************************
    * Create an object of ArrayViewBandwidth object(float,false)                  *
    *******************************************************************************/
    ArrayBandwidth<float> myInstance;

    /*******************************************************************************
    * Initialize the options of the sample                                         *
    *******************************************************************************/
    if(myInstance.initialize() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    /*******************************************************************************
    * Parse command line options                                                   *
    *******************************************************************************/
    if(myInstance.sampleArgs->parseCommandLine(argc,argv) != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    /*******************************************************************************
    * Print all devices                                                            *
    *******************************************************************************/
    myInstance.sampleArgs->printDeviceList();

    /*******************************************************************************
    * Set default accelerator                                                      *
    *******************************************************************************/
    if(myInstance.sampleArgs->setDefaultAccelerator() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    /*******************************************************************************
    * Initialize the random array of input vectors                                 *
    *******************************************************************************/
    status = myInstance.setup();
    if(status != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    /*******************************************************************************
    * Execute ArrayViewBandwidth ,including the bandwidth of array and array_view *
    *******************************************************************************/
    if(myInstance.run() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    /*******************************************************************************
    * verifyResults to make sure the result computed by the kernel is correct
    *******************************************************************************/
    if(myInstance.verifyResults() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    return 0;
}

