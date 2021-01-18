/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */
#include <boost/test/unit_test.hpp>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <turtle/mock.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS


#include <boost/core/ignore_unused.hpp>

#include <jwt-cpp/jwt.h>

#include <memory>
#include <string>
#include <chrono>
#include <algorithm>

#include "WsChannel.hpp"
#include "ILoggerMock.hpp"
#include "IVssDatabaseMock.hpp"
#include "JsonResponses.hpp"
#include "exception.hpp"

#include "Authenticator.hpp"


namespace {
  // private key used to sign JWT client token
  const std::string validPrivateKey{
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpQIBAAKCAQEAzKCKyzYnCDFazKUG7JRGbLKLYphzT+E1NGd4K/MFPmNazTTF\n"
    "0LoIZf11b6gMLOocq3D+ALwflW5BThuizU/3vpio3kFqalCI9Fnwn3kO5JXDQ0SQ\n"
    "CgGj7UF/ZNAlU2zurkbhl2C6Kr8IE8/LU9m3gtG47Hw/fFZOlgYGcDsEdkOrAW8U\n"
    "fNsOx0uWK2K1Ri8dAmSxeh3ScNu4kfvxoQwRU9uLHceE/07Lyk1LgkMhcEwLiruo\n"
    "fO9e6O4F1FQW95Mnd7V8gVwxkfNsy+JtzLyUOZfslz4XyAgSjKOGxu5mTS7FD8Pp\n"
    "473hJtFr5vF6V6NTQ85bbeo7E89FSNLfxrCNBwIDAQABAoIBAQDAFBr0sbpl2F5R\n"
    "Jr+fJ3gL5HUuccgcPVxB+rY1GwPbEkxTv6vISDhF9GteCjKTnpaW35OugOhszngC\n"
    "p7JkYyI9CPPK3UDU1xAXvq0+JNaz/1ixNhS3L97+gLLioPfIncJWWTa9cBCQu40L\n"
    "e8xywzWdWNvrMJ4vSpyt+q3kf6GqmB4hAXLJNeiXe3XwUt8d0CNkyOsUIuK/bqif\n"
    "Pgyz2aaD+q0fiyvVcQn7pPDVW9vwsng+Ypq/haKpj2pLLIH3oRWzJj6ckYpU54rs\n"
    "Q4HYJnt03e52BO6CEFtYKOdRS4aiLew46fPytPAOe32elKOodO+JZ0TZxQ0dDQ/e\n"
    "aEice/5hAoGBAP8QFmMzxyBwcqjm9Zf+PFCFvKbwXD+ybeqLcoOsnTf/RaqTIIFF\n"
    "daL5Mlk0vhuGuZFhvDCzsrttEwS/gRaBfRASqP1gyFDVSq6O6xV4Tf3t/fBa2HjN\n"
    "eNVLSzyEcWFhNUZEDgZtgBKr6/Ok5TzYkNvJ+gppMAW9hA3iNdWyMNfZAoGBAM1h\n"
    "A8jWDef5kMmYyE0K5U1z5Cg2dCayRZbFZmp1rQB5U1T+K9LTwOBBjwOJu2HLHhIw\n"
    "Rt09dnTDXUf2wlF3WNN9AIEldEOmtEd5Gs7SjchPPbjOGQyiqMBDBEhAy5pkPp08\n"
    "qHXXCBJUZao4ImLYKjYqbUNOec+5V6Ny/0CKg1/fAoGBAO8VLlcIttOyc9fcvkMd\n"
    "vX2hDofQ8DeI0j0zP0Er8ScHMk9EoAhsimsceVRi+vwkWhdrbJKeLqA/Cr+9novx\n"
    "DsCdLShsqvgSJnHfZ351iW3HwuukzBrYRzZv4HM2lmy4SM63hgoCZDWcT4zPeU2C\n"
    "lq5e8fEGTkxjK8Az1VCdOelpAoGBAI9m7f2NeKhA2Zfp1fH1aaZrBSQO4YsjbvOX\n"
    "Yatz/xgVntn5nx/WOxZasEEIKo5eBOEuVEymXc+pmbhl08iOTLde0LtcK5IRFE/T\n"
    "f6Rp4BW9PpuLTHJGIQ4dvR+2HnPvCsk/UWD2g+xIgbQY/emGhfLMLP6SDPu9rjOy\n"
    "WAf4r0KBAoGAPssbYOLcBBEBjVB4qM1OQP9lruDj/efXSOZRn94RPWYnOZoQtqaA\n"
    "JTFeNK0SdGYGlUub7NEMEbOys+PVdd10io5KgxEkFAW56LGk3cOJELFGmO2VQVlR\n"
    "pYLfeRzUnz0uxPWlRJhhXUHIKh2FuWyYturgkiDYSX1uCSCCCAsy5PA=\n"
    "-----END RSA PRIVATE KEY-----\n"
  };

  // public key generated from 'validPrivateKey' content
  const std::string validPubKey{
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzKCKyzYnCDFazKUG7JRG\n"
    "bLKLYphzT+E1NGd4K/MFPmNazTTF0LoIZf11b6gMLOocq3D+ALwflW5BThuizU/3\n"
    "vpio3kFqalCI9Fnwn3kO5JXDQ0SQCgGj7UF/ZNAlU2zurkbhl2C6Kr8IE8/LU9m3\n"
    "gtG47Hw/fFZOlgYGcDsEdkOrAW8UfNsOx0uWK2K1Ri8dAmSxeh3ScNu4kfvxoQwR\n"
    "U9uLHceE/07Lyk1LgkMhcEwLiruofO9e6O4F1FQW95Mnd7V8gVwxkfNsy+JtzLyU\n"
    "OZfslz4XyAgSjKOGxu5mTS7FD8Pp473hJtFr5vF6V6NTQ85bbeo7E89FSNLfxrCN\n"
    "BwIDAQAB\n"
    "-----END PUBLIC KEY-----\n"
  };

    // common resources for tests
  std::shared_ptr<ILoggerMock> logMock;

  std::unique_ptr<Authenticator> auth;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      logMock = std::make_shared<ILoggerMock>();

      auth = std::make_unique<Authenticator>(logMock, std::string(""), std::string("RS256"));
    }
    ~TestSuiteFixture() {
      logMock.reset();
      auth.reset();
    }
  };
}

// Define name of test suite and define test suite fixture for pre and post test handling
BOOST_FIXTURE_TEST_SUITE(AuthenticatorTests, TestSuiteFixture)

BOOST_AUTO_TEST_CASE(Given_GoodToken_When_Validate_Shall_ValidateTokenSuccessfully)
{
  WsChannel channel;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};

  picojson::value picoJson;
  picojson::parse(picoJson,
                  R"({"Vehicle.Drivetrain.*" : "r", "Vehicle.Drivetrain.Transmission.*" : "rw"})");

  auto currTime = std::chrono::system_clock::now();
  auto exprTime = std::chrono::system_clock::now() + std::chrono::hours(24);
  // create valid token
  auto token = jwt::create()
    // header
    .set_type("JWT")
    .set_algorithm("RS256")
    // payload
    .set_subject("Example JWT")
    .set_issuer("Eclipse KUKSA")
    .set_issued_at(currTime)
    .set_expires_at(exprTime)

    .set_payload_claim("kuksa-vss", jwt::claim(picoJson))

    // signature
    .sign(jwt::algorithm::rs256{validPubKey, validPrivateKey});

  // execute

  auth->updatePubKey(validPubKey);
  auto res = auth->validate(channel, token);

  // verify

  // verify that returned TTL value is same as expiry time in token
  BOOST_TEST(res == std::chrono::time_point_cast<std::chrono::seconds>(exprTime).time_since_epoch().count());
}


BOOST_AUTO_TEST_CASE(Given_BadToken_When_Validate_Shall_ReturnError)
{
  WsChannel channel;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};

  picojson::value picoJson;
  picojson::parse(picoJson,
                  R"({"Vehicle.Drivetrain.*" : "r", "Vehicle.Drivetrain.Transmission.*" : "rw"})");

  // create valid token
  auto token = jwt::create()
    // header
    .set_type("JWT")
    .set_algorithm("RS256")
    // payload
    .set_subject("Example JWT")
    .set_issuer("Eclipse KUKSA")
    .set_issued_at(std::chrono::system_clock::now())
    .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))

    .set_payload_claim("kuksa-vss", jwt::claim(picoJson))

    // signature
    .sign(jwt::algorithm::rs256{validPubKey, validPrivateKey});

  // execute

  auth->updatePubKey(validPubKey);

  // change something in token so it fails verification
  token[10] = 'a';

  auto res = auth->validate(channel, token);

  // verify

  // verify that TTL is -1 which represents not authorized token
  BOOST_TEST(res == -1);
}

BOOST_AUTO_TEST_SUITE_END()
