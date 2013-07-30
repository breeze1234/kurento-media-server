/*
 * server_test.cpp - Kurento Media Server
 *
 * Copyright (C) 2013 Kurento
 * Contact: Miguel París Díaz <mparisdiaz@gmail.com>
 * Contact: José Antonio Santos Cadenas <santoscadenas@kurento.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "server_test_base.hpp"

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#define BOOST_TEST_MODULE ServerTest

#include <sys/socket.h>
#include <arpa/inet.h>

#include "MediaServerService.h"

#include <transport/TSocket.h>
#include <transport/TBufferTransports.h>
#include <protocol/TBinaryProtocol.h>

#include <gst/gst.h>

#include "media_config_loader.hpp"
#include "mediaServer_constants.h"
#include "mediaServer_types.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace kurento;

#define GST_CAT_DEFAULT _server_test_
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "server_test"

BOOST_AUTO_TEST_SUITE ( server_test_suite )

static void
check_version (kurento::MediaServerServiceClient client)
{
  int32_t v;
  int32_t gotVersion;
  mediaServerConstants *c;

  c = new mediaServerConstants();
  v = c->VERSION;
  delete c;

  gotVersion = client.getVersion();
  BOOST_CHECK_EQUAL (gotVersion, v);
}

static void
check_same_token (kurento::MediaServerServiceClient client)
{
  MediaObject mediaManager = MediaObject();
  MediaObject mo = MediaObject();

  client.createMediaManager (mediaManager, 0);

  client.createMixer (mo, mediaManager, MixerType::type::MAIN_MIXER);
  BOOST_CHECK_EQUAL (mediaManager.token, mo.token);

  client.createSdpEndPoint (mo, mediaManager, SdpEndPointType::type::RTP_END_POINT);
  BOOST_CHECK_EQUAL (mediaManager.token, mo.token);

  client.createSdpEndPoint (mo, mediaManager, SdpEndPointType::type::WEBRTC_END_POINT);
  BOOST_CHECK_EQUAL (mediaManager.token, mo.token);

  client.release (mediaManager);
}

static void
check_use_released_media_manager (kurento::MediaServerServiceClient client)
{
  MediaObject mediaManager = MediaObject();
  MediaObject mo = MediaObject();

  client.createMediaManager (mediaManager, 0);
  client.release (mediaManager);
  BOOST_CHECK_THROW (client.createMixer (mo, mediaManager, MixerType::type::MAIN_MIXER), MediaObjectNotFoundException);
}

static void
check_parent (kurento::MediaServerServiceClient client)
{
  MediaObject mediaManager = MediaObject();
  MediaObject mo = MediaObject();
  MediaObject parent = MediaObject();

  client.createMediaManager (mediaManager, 0);

  client.createMixer (mo, mediaManager, MixerType::type::MAIN_MIXER);
  client.getParent (parent, mo);
  BOOST_CHECK_EQUAL (mediaManager.id, parent.id);

  client.release (mediaManager);
}

static void
check_media_manager_no_parent (kurento::MediaServerServiceClient client)
{
  MediaObject mediaManager = MediaObject();
  MediaObject parent = MediaObject();

  GST_DEBUG ("check_media_manager_no_parent test");
  client.createMediaManager (mediaManager, 0);
  BOOST_CHECK_THROW (client.getParent (parent, mediaManager), NoParentException);

  client.release (mediaManager);
}

static void
check_sdp_end_point (kurento::MediaServerServiceClient client)
{
  MediaObject mediaManager = MediaObject();
  MediaObject sdpEp = MediaObject();
  std::string out;

  client.createMediaManager (mediaManager, 0);

  client.createSdpEndPoint (sdpEp, mediaManager, SdpEndPointType::type::RTP_END_POINT);
  client.generateOffer (out, sdpEp);
  GST_INFO ("RTP EndPoint generateOffer: %s", out.c_str () );
  client.processOffer (out, sdpEp, "");
  GST_INFO ("RTP EndPoint processOffer: %s", out.c_str () );
  client.processAnswer (out, sdpEp, "");
  GST_INFO ("RTP EndPoint processAnswer: %s", out.c_str () );

  client.createSdpEndPointWithFixedSdp (sdpEp, mediaManager, SdpEndPointType::type::RTP_END_POINT, "");
  client.generateOffer (out, sdpEp);
  GST_INFO ("RTP EndPoint generateOffer: %s", out.c_str () );
  client.processOffer (out, sdpEp, "");
  GST_INFO ("RTP EndPoint processOffer: %s", out.c_str () );
  client.processAnswer (out, sdpEp, "");
  GST_INFO ("RTP EndPoint processAnswer: %s", out.c_str () );

  client.createSdpEndPoint (sdpEp, mediaManager, SdpEndPointType::type::WEBRTC_END_POINT);
  client.generateOffer (out, sdpEp);
  GST_INFO ("WebRTC EndPoint generateOffer: %s", out.c_str () );
  client.processOffer (out, sdpEp, "");
  GST_INFO ("WebRTC EndPoint processOffer: %s", out.c_str () );
  client.processAnswer (out, sdpEp, "");
  GST_INFO ("WebRTC EndPoint processAnswer: %s", out.c_str () );

  client.createSdpEndPointWithFixedSdp (sdpEp, mediaManager, SdpEndPointType::type::WEBRTC_END_POINT, "");
  client.generateOffer (out, sdpEp);
  GST_INFO ("WebRTC EndPoint generateOffer: %s", out.c_str () );
  client.processOffer (out, sdpEp, "");
  GST_INFO ("WebRTC EndPoint processOffer: %s", out.c_str () );
  client.processAnswer (out, sdpEp, "");
  GST_INFO ("WebRTC EndPoint processAnswer: %s", out.c_str () );

  client.release (mediaManager);
}

static void
client_side ()
{
  boost::shared_ptr<TSocket> socket (new TSocket (MEDIA_SERVER_ADDRESS, MEDIA_SERVER_SERVICE_PORT) );
  boost::shared_ptr<TTransport> transport (new TFramedTransport (socket) );
  boost::shared_ptr<TProtocol> protocol (new TBinaryProtocol (transport) );
  kurento::MediaServerServiceClient client (protocol);

  transport->open ();

  check_version (client);
  check_use_released_media_manager (client);
  check_same_token (client);
  check_parent (client);
  check_media_manager_no_parent (client);
  check_sdp_end_point (client);

  transport->close ();
}

BOOST_AUTO_TEST_CASE ( server_test )
{
  gst_init (NULL, NULL);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
      GST_DEFAULT_NAME);

  START_SERVER_TEST();
  client_side();
  STOP_SERVER_TEST();
}

BOOST_AUTO_TEST_SUITE_END()