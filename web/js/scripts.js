$.showAlert = function(msg) {

	$("#alertMsg").html(msg);
	$("#alert").fadeIn(800).delay(500).fadeOut(2000);
}

$(document).ready(function(){

	var oUrl = $("#urlInput");
	var oLifetime = $("#lifetime");
	var oExpitetime = $("#expiretime");
	var oSecure = $("#secure");

	oExpitetime.keydown(function (event) {
    	if (event.shiftKey == true) { event.preventDefault(); }
    	if ((event.keyCode >= 48 && event.keyCode <= 57) || (event.keyCode >= 96 && event.keyCode <= 105) || event.keyCode == 8 || event.keyCode == 9 || event.keyCode == 37 || event.keyCode == 39 || event.keyCode == 46) {
			oLifetime.attr("checked", false);
    	} else { event.preventDefault(); }
	});

	$("#urlSubmit").click(function(e) {

		if(1 > oUrl.val().length) { 
			$.showAlert("Missing URL");
		} else {
			var url_validate = /(ftp|http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/;
			if(false == url_validate.test(oUrl.val())) {
				
				$.showAlert("Not valid URL");

			} else {

				if(true == oLifetime.is(":checked") && 0 < oExpitetime.val().length) {
						
					$.showAlert("Not allow both value : " + oExpitetime.val());
					oExpitetime.focus();
					return false;
				}

				var url = oUrl.val();
				var lifetime = 0;
				var secure = 0;
                var expiretime = 0;

				if(true == oLifetime.is(":checked")) {
					lifetime = 0;
				} else {
					lifetime = 1;
                    expiretime = oExpitetime.val();
				}
	
				if(true == oSecure.is(":checked")) {
					secure = 1;
				}

                //var callUrl = "url="+url+"&lifetime="+lifetime+"&secure="+secure+"&expiretime="+expiretime;
				var request = $.ajax({
							url: "/api/insert.html",
							type: "POST",
							data: { 
										url: url,
										lifetime: lifetime,
										secure: secure,
										expiretime: expiretime
									},
							dataType: "text"
							});

				request.done(function( msg ) {
								e.preventDefault();
								$('#actionModalTitle').html('Complete');
								$('#actionModalMsg').html('Added!!');
								//$('#actionModalMsg').html(msg);
								$('#actionModal').modal({ keyboard: false });
								setTimeout('location.href="/";', 1200);
							});

				request.fail(function( jqXHR, textStatus ) {
								$.showAlert("can not insert data : "+ textStatus );
							});
			}
		}

	event.preventDefault();
	});

	$( document ).tooltip();

	$("#backtohome").click(function(e) {

		$(location).attr('href',"/");
		

	});

	$("#showinfo").click(function(e) {

		e.preventDefault();
		var request = $.ajax({ url: "/api/info.html", type: "GET" , dataType: "text"});

		request.done(function( msg ) {
					$('#actionModalTitle').html('Information');
					$('#actionModalContent').css('width','1200px');
					$('#actionModalContent').css('margin-left','-300px');
					$('#actionModalMsg').html(msg);
					$('#actionModal').modal({ keyboard: true});

				});

				request.fail(function( jqXHR, textStatus ) {
				$.showAlert("can not insert data : "+ textStatus );
				});
	});


	$(document).on("click", "#screenshot", function (ev) {
		window.open("/screenshot.html?tinyurl="+$(this).val());
	});

});


