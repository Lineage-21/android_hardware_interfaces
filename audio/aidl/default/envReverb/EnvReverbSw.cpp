/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstddef>
#define LOG_TAG "AHAL_EnvReverbSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "EnvReverbSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EnvReverbSw;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kEnvReverbSwImplUUID;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kEnvReverbSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<EnvReverbSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != kEnvReverbSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = EnvReverbSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string EnvReverbSw::kEffectName = "EnvReverbSw";
const EnvironmentalReverb::Capability EnvReverbSw::kCapability = {.minRoomLevelMb = -6000,
                                                                  .maxRoomLevelMb = 0,
                                                                  .minRoomHfLevelMb = -4000,
                                                                  .maxRoomHfLevelMb = 0,
                                                                  .maxDecayTimeMs = 7000,
                                                                  .minDecayHfRatioPm = 100,
                                                                  .maxDecayHfRatioPm = 2000,
                                                                  .minLevelMb = -6000,
                                                                  .maxLevelMb = 0,
                                                                  .maxDelayMs = 65,
                                                                  .maxDiffusionPm = 1000,
                                                                  .maxDensityPm = 1000};
const Descriptor EnvReverbSw::kDescriptor = {
        .common = {.id = {.type = kEnvReverbTypeUUID,
                          .uuid = kEnvReverbSwImplUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = EnvReverbSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = Capability::make<Capability::environmentalReverb>(EnvReverbSw::kCapability)};

ndk::ScopedAStatus EnvReverbSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EnvReverbSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::environmentalReverb != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    auto& erParam = specific.get<Parameter::Specific::environmentalReverb>();
    auto tag = erParam.getTag();

    switch (tag) {
        case EnvironmentalReverb::roomLevelMb: {
            RETURN_IF(mContext->setErRoomLevel(erParam.get<EnvironmentalReverb::roomLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::roomHfLevelMb: {
            RETURN_IF(
                    mContext->setErRoomHfLevel(erParam.get<EnvironmentalReverb::roomHfLevelMb>()) !=
                            RetCode::SUCCESS,
                    EX_ILLEGAL_ARGUMENT, "setRoomHfLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::decayTimeMs: {
            RETURN_IF(mContext->setErDecayTime(erParam.get<EnvironmentalReverb::decayTimeMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayTimeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::decayHfRatioPm: {
            RETURN_IF(
                    mContext->setErDecayHfRatio(
                            erParam.get<EnvironmentalReverb::decayHfRatioPm>()) != RetCode::SUCCESS,
                    EX_ILLEGAL_ARGUMENT, "setDecayHfRatioFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::levelMb: {
            RETURN_IF(mContext->setErLevel(erParam.get<EnvironmentalReverb::levelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::delayMs: {
            RETURN_IF(mContext->setErDelay(erParam.get<EnvironmentalReverb::delayMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDelayFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::diffusionPm: {
            RETURN_IF(mContext->setErDiffusion(erParam.get<EnvironmentalReverb::diffusionPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDiffusionFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::densityPm: {
            RETURN_IF(mContext->setErDensity(erParam.get<EnvironmentalReverb::densityPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDensityFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::bypass: {
            RETURN_IF(mContext->setErBypass(erParam.get<EnvironmentalReverb::bypass>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setBypassFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "EnvironmentalReverbTagNotSupported");
        }
    }
}

ndk::ScopedAStatus EnvReverbSw::getParameterSpecific(const Parameter::Id& id,
                                                     Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::environmentalReverbTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto erId = id.get<Parameter::Id::environmentalReverbTag>();
    auto erIdTag = erId.getTag();
    switch (erIdTag) {
        case EnvironmentalReverb::Id::commonTag:
            return getParameterEnvironmentalReverb(erId.get<EnvironmentalReverb::Id::commonTag>(),
                                                   specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(erIdTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "EnvironmentalReverbTagNotSupported");
    }
}

ndk::ScopedAStatus EnvReverbSw::getParameterEnvironmentalReverb(const EnvironmentalReverb::Tag& tag,
                                                                Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    EnvironmentalReverb erParam;
    switch (tag) {
        case EnvironmentalReverb::roomLevelMb: {
            erParam.set<EnvironmentalReverb::roomLevelMb>(mContext->getErRoomLevel());
            break;
        }
        case EnvironmentalReverb::roomHfLevelMb: {
            erParam.set<EnvironmentalReverb::roomHfLevelMb>(mContext->getErRoomHfLevel());
            break;
        }
        case EnvironmentalReverb::decayTimeMs: {
            erParam.set<EnvironmentalReverb::decayTimeMs>(mContext->getErDecayTime());
            break;
        }
        case EnvironmentalReverb::decayHfRatioPm: {
            erParam.set<EnvironmentalReverb::decayHfRatioPm>(mContext->getErDecayHfRatio());
            break;
        }
        case EnvironmentalReverb::levelMb: {
            erParam.set<EnvironmentalReverb::levelMb>(mContext->getErLevel());
            break;
        }
        case EnvironmentalReverb::delayMs: {
            erParam.set<EnvironmentalReverb::delayMs>(mContext->getErDelay());
            break;
        }
        case EnvironmentalReverb::diffusionPm: {
            erParam.set<EnvironmentalReverb::diffusionPm>(mContext->getErDiffusion());
            break;
        }
        case EnvironmentalReverb::densityPm: {
            erParam.set<EnvironmentalReverb::densityPm>(mContext->getErDensity());
            break;
        }
        case EnvironmentalReverb::bypass: {
            erParam.set<EnvironmentalReverb::bypass>(mContext->getErBypass());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "EnvironmentalReverbTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::environmentalReverb>(erParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> EnvReverbSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<EnvReverbSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

std::shared_ptr<EffectContext> EnvReverbSw::getContext() {
    return mContext;
}

RetCode EnvReverbSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status EnvReverbSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode EnvReverbSwContext::setErRoomLevel(int roomLevel) {
    if (roomLevel < EnvReverbSw::kCapability.minRoomLevelMb ||
        roomLevel > EnvReverbSw::kCapability.maxRoomLevelMb) {
        LOG(ERROR) << __func__ << " invalid roomLevel: " << roomLevel;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new room level
    mRoomLevel = roomLevel;
    return RetCode::SUCCESS;
}

RetCode EnvReverbSwContext::setErRoomHfLevel(int roomHfLevel) {
    if (roomHfLevel < EnvReverbSw::kCapability.minRoomHfLevelMb ||
        roomHfLevel > EnvReverbSw::kCapability.maxRoomHfLevelMb) {
        LOG(ERROR) << __func__ << " invalid roomHfLevel: " << roomHfLevel;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new room HF level
    mRoomHfLevel = roomHfLevel;
    return RetCode::SUCCESS;
}

RetCode EnvReverbSwContext::setErDecayTime(int decayTime) {
    if (decayTime < 0 || decayTime > EnvReverbSw::kCapability.maxDecayTimeMs) {
        LOG(ERROR) << __func__ << " invalid decayTime: " << decayTime;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new decay time
    mDecayTime = decayTime;
    return RetCode::SUCCESS;
}

RetCode EnvReverbSwContext::setErDecayHfRatio(int decayHfRatio) {
    if (decayHfRatio < EnvReverbSw::kCapability.minDecayHfRatioPm ||
        decayHfRatio > EnvReverbSw::kCapability.maxDecayHfRatioPm) {
        LOG(ERROR) << __func__ << " invalid decayHfRatio: " << decayHfRatio;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new decay HF ratio
    mDecayHfRatio = decayHfRatio;
    return RetCode::SUCCESS;
}

RetCode EnvReverbSwContext::setErLevel(int level) {
    if (level < EnvReverbSw::kCapability.minLevelMb ||
        level > EnvReverbSw::kCapability.maxLevelMb) {
        LOG(ERROR) << __func__ << " invalid level: " << level;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new level
    mLevel = level;
    return RetCode::SUCCESS;
}

RetCode EnvReverbSwContext::setErDelay(int delay) {
    if (delay < 0 || delay > EnvReverbSw::kCapability.maxDelayMs) {
        LOG(ERROR) << __func__ << " invalid delay: " << delay;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new delay
    mDelay = delay;
    return RetCode::SUCCESS;
}

RetCode EnvReverbSwContext::setErDiffusion(int diffusion) {
    if (diffusion < 0 || diffusion > EnvReverbSw::kCapability.maxDiffusionPm) {
        LOG(ERROR) << __func__ << " invalid diffusion: " << diffusion;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new diffusion
    mDiffusion = diffusion;
    return RetCode::SUCCESS;
}

RetCode EnvReverbSwContext::setErDensity(int density) {
    if (density < 0 || density > EnvReverbSw::kCapability.maxDensityPm) {
        LOG(ERROR) << __func__ << " invalid density: " << density;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new density
    mDensity = density;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
