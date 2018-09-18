/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "secerr.h"
#include "ssl.h"

#include "gtest_utils.h"
#include "tls_agent.h"
#include "tls_connect.h"

namespace nss_test {

static const char *kDummySni("dummy.invalid");

static void SetupESNI(const std::shared_ptr<TlsAgent>& client,
                      const std::shared_ptr<TlsAgent>& server) {
    SECKEYPrivateKey *priv = nullptr;
    SECKEYPublicKey *pub = nullptr;
    uint8_t encoded[1024];
    unsigned int encodedLen;
    SECStatus rv = SSL_GenerateESNIKeyPair(
        server->ssl_fd(),
        ssl_grp_ec_curve25519,
        &priv, &pub,
        encoded, &encodedLen,
        sizeof(encoded));
    ASSERT_EQ(SECSuccess, rv);
    ASSERT_GT(encodedLen, 0U);
    std::cerr << "Encoded length " << encodedLen << std::endl;

    rv = SSL_SetESNIKeyPair(server->ssl_fd(),
                            priv, pub,
                            ssl_grp_ec_curve25519);
    ASSERT_EQ(SECSuccess, rv);

    rv = SSL_EnableESNI(client->ssl_fd(),
                        encoded, encodedLen, kDummySni);
    ASSERT_EQ(SECSuccess, rv);
}

static void CheckSNIExtension(const DataBuffer& data) {
    TlsParser parser(data.data(), data.len());
    uint32_t tmp;
    ASSERT_TRUE(parser.Read(&tmp, 2));
    ASSERT_EQ(parser.remaining(), tmp);
    ASSERT_TRUE(parser.Read(&tmp, 1));
    ASSERT_EQ(0U, tmp); /* sni_nametype_hostname */
    DataBuffer name;
    ASSERT_TRUE(parser.ReadVariable(&name, 2));
    ASSERT_EQ(0U, parser.remaining());
    DataBuffer expected(reinterpret_cast<const uint8_t *>(kDummySni), strlen(kDummySni));
    ASSERT_EQ(expected, name);
}

TEST_P(TlsAgentTestClient13, EncodeDecodeESNIKeys) {
    EnsureInit();
    SetupESNI(agent_, agent_);
}

TEST_P(TlsConnectTls13, ConnectESNI) {
    EnsureTlsSetup();
    SetupESNI(client_, server_);
    auto filter = MakeTlsFilter<TlsExtensionCapture>(client_,
                                                     ssl_server_name_xtn);
    server_->SetSniCallback([](
        TlsAgent *agent, const SECItem* srvNameAddr,
        PRUint32 srvNameArrSize) -> int32_t {
                                EXPECT_EQ(1U, srvNameArrSize);
                                SECItem expected = {siBuffer,
                                                    reinterpret_cast<unsigned char *>(const_cast<char *>("server")), 6 };
                                EXPECT_TRUE(
                                    !SECITEM_CompareItem(&expected,
                                                         &srvNameAddr[0]));
                                return SECSuccess;
                            });
    Connect();
    CheckSNIExtension(filter->extension());
}

}
