/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class BPWriteReadLocalVariables : public ::testing::Test
{
public:
    BPWriteReadLocalVariables() = default;

    SmallTestData m_TestData;
};

TEST_F(BPWriteReadLocalVariables, ADIOS2BPWriteReadLocal1D)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname("ADIOS2BPWriteReadLocal1D.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 8;

    // Number of steps
    const size_t NSteps = 5;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

// Write test data using BP

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::Dims shape{};
            const adios2::Dims start{};
            const adios2::Dims count{Nx};

            io.DefineVariable<int32_t>("stepsGlobalValue");
            io.DefineVariable<std::string>("stepsGlobalValueString");

            io.DefineVariable<int32_t>("ranksLocalValue",
                                       {adios2::LocalValueDim});
            io.DefineVariable<std::string>("ranksLocalValueString",
                                           {adios2::LocalValueDim});

            io.DefineVariable<int8_t>("i8", shape, start, count,
                                      adios2::ConstantDims);
            io.DefineVariable<int16_t>("i16", shape, start, count,
                                       adios2::ConstantDims);
            io.DefineVariable<int32_t>("i32", shape, start, count,
                                       adios2::ConstantDims);
            io.DefineVariable<int64_t>("i64", shape, start, count,
                                       adios2::ConstantDims);

            io.DefineVariable<uint8_t>("u8", shape, start, count,
                                       adios2::ConstantDims);
            io.DefineVariable<uint16_t>("u16", shape, start, count,
                                        adios2::ConstantDims);
            io.DefineVariable<uint32_t>("u32", shape, start, count,
                                        adios2::ConstantDims);
            io.DefineVariable<uint64_t>("u64", shape, start, count,
                                        adios2::ConstantDims);

            io.DefineVariable<float>("r32", shape, start, count,
                                     adios2::ConstantDims);
            io.DefineVariable<double>("r64", shape, start, count,
                                      adios2::ConstantDims);

            io.DefineVariable<std::complex<float>>("cr32", shape, start, count,
                                                   adios2::ConstantDims);
            io.DefineVariable<std::complex<double>>("cr64", shape, start, count,
                                                    adios2::ConstantDims);
        }

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            EXPECT_EQ(bpWriter.CurrentStep(), step);

            bpWriter.BeginStep();

            bpWriter.Put<int32_t>("stepsGlobalValue", step);
            bpWriter.Put<std::string>("stepsGlobalValueString",
                                      std::to_string(step));

            bpWriter.Put<int32_t>("ranksLocalValue", mpiRank);
            bpWriter.Put<std::string>("ranksLocalValueString",
                                      std::to_string(mpiRank));

            bpWriter.Put<int8_t>("i8", currentTestData.I8.data());
            bpWriter.Put<int16_t>("i16", currentTestData.I16.data());
            bpWriter.Put<int32_t>("i32", currentTestData.I32.data());
            bpWriter.Put<int64_t>("i64", currentTestData.I64.data());
            bpWriter.Put<uint8_t>("u8", currentTestData.U8.data());
            bpWriter.Put<uint16_t>("u16", currentTestData.U16.data());
            bpWriter.Put<uint32_t>("u32", currentTestData.U32.data());
            bpWriter.Put<uint64_t>("u64", currentTestData.U64.data());
            bpWriter.Put<float>("r32", currentTestData.R32.data());
            bpWriter.Put<double>("r64", currentTestData.R64.data());
            bpWriter.Put<std::complex<float>>("cr32",
                                              currentTestData.CR32.data());
            bpWriter.Put<std::complex<double>>("cr64",
                                               currentTestData.CR64.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    // if (mpiRank == 0)
    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        adios2::Engine bpReader =
            io.Open(fname, adios2::Mode::Read, MPI_COMM_SELF);

        std::string IString;
        std::array<int8_t, Nx> I8;
        std::array<int16_t, Nx> I16;
        std::array<int32_t, Nx> I32;
        std::array<int64_t, Nx> I64;
        std::array<uint8_t, Nx> U8;
        std::array<uint16_t, Nx> U16;
        std::array<uint32_t, Nx> U32;
        std::array<uint64_t, Nx> U64;
        std::array<float, Nx> R32;
        std::array<double, Nx> R64;
        std::array<std::complex<float>, Nx> CR32;
        std::array<std::complex<double>, Nx> CR64;

        // block ID is zero by default
        const adios2::Dims start{0};
        const adios2::Dims count{Nx};
        const adios2::Box<adios2::Dims> sel(start, count);

        size_t t = 0;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t currentStep = bpReader.CurrentStep();
            EXPECT_EQ(currentStep, static_cast<size_t>(t));

            auto var_StepsGlobalValue =
                io.InquireVariable<int32_t>("stepsGlobalValue");
            auto var_StepsGlobalValueString =
                io.InquireVariable<std::string>("stepsGlobalValueString");
            auto var_RanksLocalValue =
                io.InquireVariable<int32_t>("ranksLocalValue");
            auto var_RanksLocalValueString =
                io.InquireVariable<std::string>("ranksLocalValueString");

            auto var_i8 = io.InquireVariable<int8_t>("i8");
            auto var_i16 = io.InquireVariable<int16_t>("i16");
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            auto var_i64 = io.InquireVariable<int64_t>("i64");
            auto var_u8 = io.InquireVariable<uint8_t>("u8");
            auto var_u16 = io.InquireVariable<uint16_t>("u16");
            auto var_u32 = io.InquireVariable<uint32_t>("u32");
            auto var_u64 = io.InquireVariable<uint64_t>("u64");
            auto var_r32 = io.InquireVariable<float>("r32");
            auto var_r64 = io.InquireVariable<double>("r64");
            auto var_cr32 = io.InquireVariable<std::complex<float>>("cr32");
            auto var_cr64 = io.InquireVariable<std::complex<double>>("cr64");

            // Global value
            EXPECT_TRUE(var_StepsGlobalValue);
            ASSERT_EQ(var_StepsGlobalValue.ShapeID(),
                      adios2::ShapeID::GlobalValue);
            ASSERT_EQ(var_StepsGlobalValue.Steps(), NSteps);
            ASSERT_EQ(var_StepsGlobalValue.Shape().size(), 0);
            ASSERT_EQ(var_StepsGlobalValue.Min(), 0);
            ASSERT_EQ(var_StepsGlobalValue.Max(), NSteps - 1);
            int32_t stepsGlobalValueData;
            bpReader.Get(var_StepsGlobalValue, stepsGlobalValueData,
                         adios2::Mode::Sync);
            ASSERT_EQ(stepsGlobalValueData, currentStep);

            EXPECT_TRUE(var_StepsGlobalValueString);
            ASSERT_EQ(var_StepsGlobalValueString.ShapeID(),
                      adios2::ShapeID::GlobalValue);
            ASSERT_EQ(var_StepsGlobalValueString.Steps(), NSteps);
            ASSERT_EQ(var_StepsGlobalValueString.Shape().size(), 0);
            std::string stepsGlobalValueStringDataString;
            bpReader.Get(var_StepsGlobalValueString,
                         stepsGlobalValueStringDataString, adios2::Mode::Sync);
            ASSERT_EQ(stepsGlobalValueStringDataString,
                      std::to_string(currentStep));

            // Local value
            EXPECT_TRUE(var_RanksLocalValue);
            ASSERT_EQ(var_RanksLocalValue.ShapeID(),
                      adios2::ShapeID::LocalValue);
            ASSERT_EQ(var_RanksLocalValue.Steps(), NSteps);
            ASSERT_EQ(var_RanksLocalValue.Shape()[0], mpiSize);
            ASSERT_EQ(var_RanksLocalValue.Min(), 0);
            ASSERT_EQ(var_RanksLocalValue.Max(), mpiSize - 1);
            std::vector<int32_t> rankLocalValueData;
            bpReader.Get(var_RanksLocalValue, rankLocalValueData);
            ASSERT_EQ(rankLocalValueData.size(), mpiSize);
            for (int32_t r = 0; r < rankLocalValueData.size(); ++r)
            {
                ASSERT_EQ(rankLocalValueData[r], r);
            }

            EXPECT_TRUE(var_RanksLocalValueString);
            ASSERT_EQ(var_RanksLocalValueString.ShapeID(),
                      adios2::ShapeID::LocalValue);
            ASSERT_EQ(var_RanksLocalValue.Steps(), NSteps);
            ASSERT_EQ(var_RanksLocalValue.Shape()[0], mpiSize);
            std::vector<std::string> rankLocalValueDataString;
            bpReader.Get(var_RanksLocalValueString, rankLocalValueDataString,
                         adios2::Mode::Sync);
            ASSERT_EQ(rankLocalValueData.size(), mpiSize);
            for (int32_t r = 0; r < rankLocalValueData.size(); ++r)
            {
                ASSERT_EQ(rankLocalValueDataString[r], std::to_string(r));
            }

            EXPECT_TRUE(var_i8);
            EXPECT_TRUE(var_i16);
            EXPECT_TRUE(var_i32);
            EXPECT_TRUE(var_i64);
            EXPECT_TRUE(var_u8);
            EXPECT_TRUE(var_u16);
            EXPECT_TRUE(var_u32);
            EXPECT_TRUE(var_u64);
            EXPECT_TRUE(var_cr32);
            EXPECT_TRUE(var_cr64);

            //            ASSERT_EQ(var_i8.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_i8.Steps(), NSteps);
            //            ASSERT_EQ(var_i8.Shape()[0], Nx);
            //
            //            ASSERT_EQ(var_i16.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_i16.Steps(), NSteps);
            //            ASSERT_EQ(var_i16.Shape()[0], Nx);
            //
            ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::LocalArray);
            ASSERT_EQ(var_i32.Steps(), NSteps);
            ASSERT_EQ(var_i32.Shape().size(), 0);
            //
            //            ASSERT_EQ(var_i64.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_i64.Steps(), NSteps);
            //            ASSERT_EQ(var_i64.Shape()[0], Nx);
            //
            //            ASSERT_EQ(var_u8.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_u8.Steps(), NSteps);
            //            ASSERT_EQ(var_u8.Shape()[0], Nx);
            //
            //            ASSERT_EQ(var_u16.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_u16.Steps(), NSteps);
            //            ASSERT_EQ(var_u16.Shape()[0], Nx);
            //
            //            ASSERT_EQ(var_u32.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_u32.Steps(), NSteps);
            //            ASSERT_EQ(var_u32.Shape()[0], Nx);
            //
            //            ASSERT_EQ(var_u64.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_u64.Steps(), NSteps);
            //            ASSERT_EQ(var_u64.Shape()[0], Nx);
            //
            //            ASSERT_EQ(var_cr32.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_cr32.Steps(), NSteps);
            //            ASSERT_EQ(var_cr32.Shape()[0], Nx);
            //
            //            ASSERT_EQ(var_cr64.ShapeID(),
            //            adios2::ShapeID::LocalArray);
            //            ASSERT_EQ(var_cr64.Steps(), NSteps);
            //            ASSERT_EQ(var_cr64.Shape()[0], Nx);

            // Select blockID, 0 is default
            //
            //            bpReader.Get(var_i8, I8.data());
            //            bpReader.Get(var_i16, I16.data());
            //            bpReader.Get(var_i32, I32.data());
            //            bpReader.Get(var_i64, I64.data());
            //
            //            bpReader.Get(var_u8, U8.data());
            //            bpReader.Get(var_u16, U16.data());
            //            bpReader.Get(var_u32, U32.data());
            //            bpReader.Get(var_u64, U64.data());
            //
            //            bpReader.Get(var_r32, R32.data());
            //            bpReader.Get(var_r64, R64.data());
            //
            //            bpReader.Get(var_cr32, CR32.data());
            //            bpReader.Get(var_cr64, CR64.data());

            for (size_t b = 0; b < mpiSize; ++b)
            {
                var_i32.SetBlockID(b);
                ASSERT_EQ(var_i32.BlockID(), b);

                bpReader.Get(var_i32, I32.data());
                bpReader.PerformGets();

                SmallTestData currentTestData = generateNewSmallTestData(
                    m_TestData, static_cast<int>(currentStep),
                    static_cast<int>(b), mpiSize);

                for (size_t i = 0; i < Nx; ++i)
                {
                    std::stringstream ss;
                    ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                    std::string msg = ss.str();

                    if (var_i32)
                        ASSERT_EQ(I32[i], currentTestData.I32[i]) << msg;
                }
            }

            bpReader.EndStep();

            //
            //                if (var_i8)
            //                    EXPECT_EQ(I8[i], currentTestData.I8[i]) <<
            //                    msg;
            //                if (var_i16)
            //                    EXPECT_EQ(I16[i], currentTestData.I16[i])
            //                    << msg;
            //                if (var_i32)
            //                    EXPECT_EQ(I32[i], currentTestData.I32[i])
            //                    << msg;
            //                if (var_i64)
            //                    EXPECT_EQ(I64[i], currentTestData.I64[i])
            //                    << msg;
            //
            //                if (var_u8)
            //                    EXPECT_EQ(U8[i], currentTestData.U8[i]) <<
            //                    msg;
            //                if (var_u16)
            //                    EXPECT_EQ(U16[i], currentTestData.U16[i])
            //                    << msg;
            //                if (var_u32)
            //                    EXPECT_EQ(U32[i], currentTestData.U32[i])
            //                    << msg;
            //                if (var_u64)
            //                    EXPECT_EQ(U64[i], currentTestData.U64[i])
            //                    << msg;
            //                if (var_r32)
            //                    EXPECT_EQ(R32[i], currentTestData.R32[i])
            //                    << msg;
            //                if (var_r64)
            //                    EXPECT_EQ(R64[i], currentTestData.R64[i])
            //                    << msg;
            //
            //                if (var_cr32)
            //                    EXPECT_EQ(CR32[i],
            //                    currentTestData.CR32[i]) << msg;
            //                if (var_cr64)
            //                    EXPECT_EQ(CR64[i],
            //                    currentTestData.CR64[i]) << msg;

            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
